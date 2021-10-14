#pragma once

#include <mutex>
#include <shared_mutex>


namespace atoms
{
template < class T, class Mutex = std::mutex >
class Guarded
{
private:
    class GuardedHandle
    {
    public:
        T * operator->()
        {
            return _data;
        }

    private:
        friend class Guarded;

        GuardedHandle( Guarded & guarded ) : _lock( guarded._mutex ), _data( &guarded._data ) {}

        std::lock_guard< Mutex > _lock;
        T * _data;
    };

public:
    template < typename... Args >
    Guarded( Args &&... args ) : _data( std::forward< Args >( args )... )
    {}

    GuardedHandle operator->()
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
