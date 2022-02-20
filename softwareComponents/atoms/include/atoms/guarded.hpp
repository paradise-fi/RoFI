#pragma once

#include <mutex>
#include <shared_mutex>
#include <type_traits>


namespace atoms
{
template < class T, class Mutex = std::mutex >
class Guarded
{
private:
    template < bool IsConst >
    class GuardedHandle
    {
        using GuardedType = std::conditional_t< IsConst, const Guarded, Guarded >;
        using DataPointer = std::conditional_t< IsConst, const T *, T * >;

    public:
        GuardedHandle( const GuardedHandle & ) = delete;
        GuardedHandle & operator=( const GuardedHandle & ) = delete;

        DataPointer operator->() const noexcept
        {
            return _data;
        }

    private:
        friend class Guarded;

        explicit GuardedHandle( GuardedType & guarded ) : _lock( guarded._mutex ), _data( &guarded._data ) {}

        std::lock_guard< Mutex > _lock;
        DataPointer _data;
    };

public:
    template < typename... Args >
    explicit Guarded( Args &&... args ) : _data( std::forward< Args >( args )... )
    {}

    GuardedHandle< true > operator->() const
    {
        return GuardedHandle< true >( *this );
    }
    GuardedHandle< false > operator->()
    {
        return GuardedHandle< false >( *this );
    }

    template < typename F >
    auto visit( F && f )
    {
        using ReturnT = decltype( f( _data ) );
        static_assert( !std::is_lvalue_reference_v< ReturnT > );
        static_assert( !std::is_rvalue_reference_v< ReturnT > );

        auto _lock = std::lock_guard( _mutex );
        return f( _data );
    }

    template < typename F >
    auto visit( F && f ) const
    {
        using ReturnT = decltype( f( _data ) );
        static_assert( !std::is_lvalue_reference_v< ReturnT > );
        static_assert( !std::is_rvalue_reference_v< ReturnT > );

        auto _lock = std::lock_guard( _mutex );
        return f( _data );
    }

    template < typename F >
    auto visit_shared( F && f ) const
    {
        using ReturnT = decltype( f( _data ) );
        static_assert( !std::is_lvalue_reference_v< ReturnT > );
        static_assert( !std::is_rvalue_reference_v< ReturnT > );

        auto _lock = std::shared_lock( _mutex );
        return f( _data );
    }

    template < typename... Args >
    void replace( Args &&... args )
    {
        visit( [ & ]( T & value ) { value = T( std::forward< Args >( args )... ); } );
    }

    T copy() const
    {
        return visit( []( T copy ) { return copy; } );
    }

private:
    mutable Mutex _mutex;
    T _data;
};

} // namespace atoms
