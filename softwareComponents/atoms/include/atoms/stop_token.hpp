//
// Taken from https://github.com/josuttis/jthread (commit 0fa8d39)
//

#pragma once
// <stop_token> header

#include <atomic>
#include <thread>
#include <type_traits>
#include <utility>
#ifdef SAFE
#include <iostream>
#endif

#if defined( __x86_64__ ) || defined( _M_X64 )
#include <immintrin.h>
#endif


namespace atoms
{
namespace detail
{
    inline void _spin_yield() noexcept
    {
        // TODO: Platform-specific code here
#if defined( __x86_64__ ) || defined( _M_X64 )
        _mm_pause();
#else
        error "undefined";
#endif
    }


    //-----------------------------------------------
    // internal types for shared stop state
    //-----------------------------------------------

    struct _stop_callback_base
    {
        void ( *_callback_ )( _stop_callback_base * ) = nullptr;

        _stop_callback_base * _next_ = nullptr;
        _stop_callback_base ** _prev_ = nullptr;
        bool * _isRemoved_ = nullptr;
        std::atomic< bool > _callbackFinishedExecuting_{ false };

        void _execute() noexcept
        {
            _callback_( this );
        }

    protected:
        // it shall only by us who deletes this
        // (workaround for virtual _execute() and destructor)
        ~_stop_callback_base() = default;
    };

    struct _stop_state
    {
    public:
        void _add_token_reference() noexcept
        {
            _state_.fetch_add( _token_ref_increment, std::memory_order_relaxed );
        }

        void _remove_token_reference() noexcept
        {
            auto _oldState = _state_.fetch_sub( _token_ref_increment, std::memory_order_acq_rel );
            if ( _oldState < ( _token_ref_increment + _source_ref_increment ) ) {
                delete this;
            }
        }

        void _add_source_reference() noexcept
        {
            _state_.fetch_add( _source_ref_increment, std::memory_order_relaxed );
        }

        void _remove_source_reference() noexcept
        {
            auto _oldState = _state_.fetch_sub( _source_ref_increment, std::memory_order_acq_rel );
            if ( _oldState < ( _token_ref_increment + _source_ref_increment ) ) {
                delete this;
            }
        }

        bool _request_stop() noexcept
        {
            if ( !_try_lock_and_signal_until_signalled() ) {
                // Stop has already been requested.
                return false;
            }

            // Set the 'stop_requested' signal and acquired the lock.

            _signallingThread_ = std::this_thread::get_id();

            while ( _head_ != nullptr ) {
                // Dequeue the head of the queue
                auto * _cb = _head_;
                _head_ = _cb->_next_;
                const bool anyMore = _head_ != nullptr;
                if ( anyMore ) {
                    _head_->_prev_ = &_head_;
                }
                // Mark this item as removed from the list.
                _cb->_prev_ = nullptr;

                // Don't hold lock while executing callback
                // so we don't block other threads from deregistering callbacks.
                _unlock();

                // TRICKY: Need to store a flag on the stack here that the callback
                // can use to signal that the destructor was executed inline
                // during the call. If the destructor was executed inline then
                // it's not safe to dereference _cb after _execute() returns.
                // If the destructor runs on some other thread then the other
                // thread will block waiting for this thread to signal that the
                // callback has finished executing.
                bool _isRemoved = false;
                _cb->_isRemoved_ = &_isRemoved;

                _cb->_execute();

                if ( !_isRemoved ) {
                    _cb->_isRemoved_ = nullptr;
                    _cb->_callbackFinishedExecuting_.store( true, std::memory_order_release );
                }

                if ( !anyMore ) {
                    // This was the last item in the queue when we dequeued it.
                    // No more items should be added to the queue after we have
                    // marked the state as interrupted, only removed from the queue.
                    // Avoid acquring/releasing the lock in this case.
                    return true;
                }

                _lock();
            }

            _unlock();

            return true;
        }

        bool _is_stop_requested() noexcept
        {
            return _is_stop_requested( _state_.load( std::memory_order_acquire ) );
        }

        bool _is_stop_requestable() noexcept
        {
            return _is_stop_requestable( _state_.load( std::memory_order_acquire ) );
        }

        bool _try_add_callback( _stop_callback_base * _cb,
                                bool _incrementRefCountIfSuccessful ) noexcept
        {
            std::uint64_t _oldState;
            goto _load_state;
            do {
                goto _check_state;
                do {
                    _spin_yield();
                _load_state:
                    _oldState = _state_.load( std::memory_order_acquire );
                _check_state:
                    if ( _is_stop_requested( _oldState ) ) {
                        _cb->_execute();
                        return false;
                    }
                    else if ( !_is_stop_requestable( _oldState ) ) {
                        return false;
                    }
                } while ( _is_locked( _oldState ) );
            } while ( !_state_.compare_exchange_weak( _oldState,
                                                      _oldState | _locked_flag,
                                                      std::memory_order_acquire ) );

            // Push callback onto callback list.
            _cb->_next_ = _head_;
            if ( _cb->_next_ != nullptr ) {
                _cb->_next_->_prev_ = &_cb->_next_;
            }
            _cb->_prev_ = &_head_;
            _head_ = _cb;

            if ( _incrementRefCountIfSuccessful ) {
                _unlock_and_increment_token_ref_count();
            }
            else {
                _unlock();
            }

            // Successfully added the callback.
            return true;
        }

        void _remove_callback( _stop_callback_base * _cb ) noexcept
        {
            _lock();

            if ( _cb->_prev_ != nullptr ) {
                // Still registered, not yet executed
                // Just remove from the list.
                *_cb->_prev_ = _cb->_next_;
                if ( _cb->_next_ != nullptr ) {
                    _cb->_next_->_prev_ = _cb->_prev_;
                }

                _unlock_and_decrement_token_ref_count();

                return;
            }

            _unlock();

            // Callback has either already executed or is executing
            // concurrently on another thread.

            if ( _signallingThread_ == std::this_thread::get_id() ) {
                // Callback executed on this thread or is still currently executing
                // and is deregistering itself from within the callback.
                if ( _cb->_isRemoved_ != nullptr ) {
                    // Currently inside the callback, let the _request_stop() method
                    // know the object is about to be destructed and that it should
                    // not try to access the object when the callback returns.
                    *_cb->_isRemoved_ = true;
                }
            }
            else {
                // Callback is currently executing on another thread,
                // block until it finishes executing.
                while ( !_cb->_callbackFinishedExecuting_.load( std::memory_order_acquire ) ) {
                    _spin_yield();
                }
            }

            _remove_token_reference();
        }

    private:
        static bool _is_locked( std::uint64_t _state ) noexcept
        {
            return ( _state & _locked_flag ) != 0;
        }

        static bool _is_stop_requested( std::uint64_t _state ) noexcept
        {
            return ( _state & _stop_requested_flag ) != 0;
        }

        static bool _is_stop_requestable( std::uint64_t _state ) noexcept
        {
            // Interruptible if it has already been interrupted or if there are
            // still interrupt_source instances in existence.
            return _is_stop_requested( _state ) || ( _state >= _source_ref_increment );
        }

        bool _try_lock_and_signal_until_signalled() noexcept
        {
            std::uint64_t _oldState = _state_.load( std::memory_order_acquire );
            do {
                if ( _is_stop_requested( _oldState ) )
                    return false;
                while ( _is_locked( _oldState ) ) {
                    _spin_yield();
                    _oldState = _state_.load( std::memory_order_acquire );
                    if ( _is_stop_requested( _oldState ) )
                        return false;
                }
            } while (
                    !_state_.compare_exchange_weak( _oldState,
                                                    _oldState | _stop_requested_flag | _locked_flag,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_acquire ) );
            return true;
        }

        void _lock() noexcept
        {
            auto _oldState = _state_.load( std::memory_order_relaxed );
            do {
                while ( _is_locked( _oldState ) ) {
                    _spin_yield();
                    _oldState = _state_.load( std::memory_order_relaxed );
                }
            } while ( !_state_.compare_exchange_weak( _oldState,
                                                      _oldState | _locked_flag,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed ) );
        }

        void _unlock() noexcept
        {
            _state_.fetch_sub( _locked_flag, std::memory_order_release );
        }

        void _unlock_and_increment_token_ref_count() noexcept
        {
            _state_.fetch_sub( _locked_flag - _token_ref_increment, std::memory_order_release );
        }

        void _unlock_and_decrement_token_ref_count() noexcept
        {
            auto _oldState = _state_.fetch_sub( _locked_flag + _token_ref_increment,
                                                std::memory_order_acq_rel );
            // Check if new state is less than _token_ref_increment which would
            // indicate that this was the last reference.
            if ( _oldState < ( _locked_flag + _token_ref_increment + _token_ref_increment ) ) {
                delete this;
            }
        }

        static constexpr std::uint64_t _stop_requested_flag = 1u;
        static constexpr std::uint64_t _locked_flag = 2u;
        static constexpr std::uint64_t _token_ref_increment = 4u;
        static constexpr std::uint64_t _source_ref_increment = static_cast< std::uint64_t >( 1u )
                                                            << 33u;

        // bit 0 - stop-requested
        // bit 1 - locked
        // bits 2-32 - token ref count (31 bits)
        // bits 33-63 - source ref count (31 bits)
        std::atomic< std::uint64_t > _state_{ _source_ref_increment };
        _stop_callback_base * _head_ = nullptr;
        std::thread::id _signallingThread_{};
    };

} // namespace detail


//-----------------------------------------------
// forward declarations
//-----------------------------------------------

class stop_source;
template < typename _Callback >
class stop_callback;

// std::nostopstate
// - to initialize a stop_source without shared stop state
struct nostopstate_t
{
    explicit nostopstate_t() = default;
};
inline constexpr nostopstate_t nostopstate{};


//-----------------------------------------------
// stop_token
//-----------------------------------------------

class stop_token
{
public:
    // construct:
    // - TODO: explicit?
    stop_token() noexcept : _state_( nullptr ) {}

    // copy/move/assign/destroy:
    stop_token( const stop_token & _it ) noexcept : _state_( _it._state_ )
    {
        if ( _state_ != nullptr ) {
            _state_->_add_token_reference();
        }
    }

    stop_token( stop_token && _it ) noexcept : _state_( std::exchange( _it._state_, nullptr ) ) {}

    ~stop_token()
    {
        if ( _state_ != nullptr ) {
            _state_->_remove_token_reference();
        }
    }

    stop_token & operator=( const stop_token & _it ) noexcept
    {
        if ( _state_ != _it._state_ ) {
            stop_token _tmp{ _it };
            swap( _tmp );
        }
        return *this;
    }

    stop_token & operator=( stop_token && _it ) noexcept
    {
        stop_token _tmp{ std::move( _it ) };
        swap( _tmp );
        return *this;
    }

    void swap( stop_token & _it ) noexcept
    {
        std::swap( _state_, _it._state_ );
    }

    // stop handling:
    [[nodiscard]] bool stop_requested() const noexcept
    {
        return _state_ != nullptr && _state_->_is_stop_requested();
    }

    [[nodiscard]] bool stop_possible() const noexcept
    {
        return _state_ != nullptr && _state_->_is_stop_requestable();
    }

    [[nodiscard]] friend bool operator==( const stop_token & _a, const stop_token & _b ) noexcept
    {
        return _a._state_ == _b._state_;
    }
    [[nodiscard]] friend bool operator!=( const stop_token & _a, const stop_token & _b ) noexcept
    {
        return _a._state_ != _b._state_;
    }

private:
    friend class stop_source;
    template < typename _Callback >
    friend class stop_callback;

    explicit stop_token( detail::_stop_state * _state ) noexcept : _state_( _state )
    {
        if ( _state_ != nullptr ) {
            _state_->_add_token_reference();
        }
    }

    detail::_stop_state * _state_;
};


//-----------------------------------------------
// stop_source
//-----------------------------------------------

class stop_source
{
public:
    stop_source() : _state_( new detail::_stop_state() ) {}

    explicit stop_source( nostopstate_t ) noexcept : _state_( nullptr ) {}

    ~stop_source()
    {
        if ( _state_ != nullptr ) {
            _state_->_remove_source_reference();
        }
    }

    stop_source( const stop_source & _other ) noexcept : _state_( _other._state_ )
    {
        if ( _state_ != nullptr ) {
            _state_->_add_source_reference();
        }
    }

    stop_source( stop_source && _other ) noexcept
            : _state_( std::exchange( _other._state_, nullptr ) )
    {}

    stop_source & operator=( stop_source && _other ) noexcept
    {
        stop_source _tmp{ std::move( _other ) };
        swap( _tmp );
        return *this;
    }

    stop_source & operator=( const stop_source & _other ) noexcept
    {
        if ( _state_ != _other._state_ ) {
            stop_source _tmp{ _other };
            swap( _tmp );
        }
        return *this;
    }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        return _state_ != nullptr && _state_->_is_stop_requested();
    }

    [[nodiscard]] bool stop_possible() const noexcept
    {
        return _state_ != nullptr;
    }

    bool request_stop() noexcept
    {
        if ( _state_ != nullptr ) {
            return _state_->_request_stop();
        }
        return false;
    }

    [[nodiscard]] stop_token get_token() const noexcept
    {
        return stop_token{ _state_ };
    }

    void swap( stop_source & _other ) noexcept
    {
        std::swap( _state_, _other._state_ );
    }

    [[nodiscard]] friend bool operator==( const stop_source & _a, const stop_source & _b ) noexcept
    {
        return _a._state_ == _b._state_;
    }
    [[nodiscard]] friend bool operator!=( const stop_source & _a, const stop_source & _b ) noexcept
    {
        return _a._state_ != _b._state_;
    }

private:
    detail::_stop_state * _state_;
};


//-----------------------------------------------
// stop_callback
//-----------------------------------------------

template < typename _Callback >
// requires Destructible<_Callback> && Invocable<_Callback>
class [[nodiscard]] stop_callback : private detail::_stop_callback_base
{
public:
    using callback_type = _Callback;

    template < typename _CB,
               std::enable_if_t< std::is_constructible_v< _Callback, _CB >, int > = 0 >
    // requires Constructible<Callback, C>
    explicit stop_callback( const stop_token & _token, _CB && _cb ) noexcept(
            std::is_nothrow_constructible_v< _Callback, _CB > )
            : detail::_stop_callback_base{ []( detail::_stop_callback_base * _that ) noexcept {
                static_cast< stop_callback * >( _that )->_execute();
            } }
            , _state_( nullptr )
            , _cb_( static_cast< _CB && >( _cb ) )
    {
        if ( _token._state_ != nullptr && _token._state_->_try_add_callback( this, true ) ) {
            _state_ = _token._state_;
        }
    }

    template < typename _CB,
               std::enable_if_t< std::is_constructible_v< _Callback, _CB >, int > = 0 >
    // requires Constructible<Callback, C>
    explicit stop_callback( stop_token && _token, _CB && _cb ) noexcept(
            std::is_nothrow_constructible_v< _Callback, _CB > )
            : detail::_stop_callback_base{ []( detail::_stop_callback_base * _that ) noexcept {
                static_cast< stop_callback * >( _that )->_execute();
            } }
            , _state_( nullptr )
            , _cb_( static_cast< _CB && >( _cb ) )
    {
        if ( _token._state_ != nullptr && _token._state_->_try_add_callback( this, false ) ) {
            _state_ = std::exchange( _token._state_, nullptr );
        }
    }

    ~stop_callback()
    {
#ifdef SAFE
        if ( _inExecute_.load() ) {
            std::cerr << "*** OOPS: ~stop_callback() while callback executed\n";
        }
#endif
        if ( _state_ != nullptr ) {
            _state_->_remove_callback( this );
        }
    }

    stop_callback & operator=( const stop_callback & ) = delete;
    stop_callback & operator=( stop_callback && ) = delete;
    stop_callback( const stop_callback & ) = delete;
    stop_callback( stop_callback && ) = delete;

private:
    void _execute() noexcept
    {
        // Executed in a noexcept context
        // If it throws then we call std::terminate().
#ifdef SAFE
        _inExecute_.store( true );
        _cb_();
        _inExecute_.store( false );
#else
        _cb_();
#endif
    }

    detail::_stop_state * _state_;
    _Callback _cb_;
#ifdef SAFE
    std::atomic< bool > _inExecute_{ false };
#endif
};

template < typename _Callback >
stop_callback( stop_token, _Callback ) -> stop_callback< _Callback >;

} // namespace atoms
