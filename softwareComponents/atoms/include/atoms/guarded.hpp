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

        GuardedHandle( GuardedType & guarded ) : _lock( guarded._mutex ), _data( &guarded._data ) {}

        std::lock_guard< Mutex > _lock;
        DataPointer _data;
    };

public:
    template < typename... Args >
    Guarded( Args &&... args ) : _data( std::forward< Args >( args )... )
    {}

    GuardedHandle< true > operator->() const
    {
        return { *this };
    }
    GuardedHandle< false > operator->()
    {
        return { *this };
    }

    template < typename F >
    auto visit( F && f )
    {
        auto _lock = std::lock_guard( _mutex );
        return f( _data );
    }

    template < typename F >
    auto visit( F && f ) const
    {
        auto _lock = std::lock_guard( _mutex );
        return f( _data );
    }

    template < typename F >
    auto visit_shared( F && f ) const
    {
        auto _lock = std::shared_lock( _mutex );
        return f( _data );
    }

private:
    mutable Mutex _mutex;
    T _data;
};

} // namespace atoms
