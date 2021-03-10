#pragma once

#include <cassert>
#include <mutex>
#include <type_traits>
#include <vector>


namespace rofi::hal
{
template < typename Callback >
class Callbacks
{
public:
    Callbacks() = default;

    Callbacks( const Callbacks & ) = delete;
    Callbacks & operator=( const Callbacks & ) = delete;
    Callbacks & operator=( Callbacks && ) = delete;

    Callbacks( Callbacks && other )
    {
        std::lock_guard< std::mutex > lock( other._mutex );
        _callbacks = std::move( other._callbacks );
    }


    void registerCallback( Callback && callback )
    {
        assert( callback );
        std::lock_guard< std::mutex > lock( _mutex );
        _callbacks.push_back( std::move( callback ) );
    }
    void registerCallback( const Callback & callback )
    {
        assert( callback );
        std::lock_guard< std::mutex > lock( _mutex );
        _callbacks.push_back( callback );
    }

    template < typename... Args >
    void callCallbacks( Args &&... args ) const
    {
        std::lock_guard< std::mutex > lock( _mutex );
        for ( const auto & callback : _callbacks )
        {
            callback( args... );
        }
    }

private:
    mutable std::mutex _mutex;
    std::vector< Callback > _callbacks;
};

} // namespace rofi::hal
