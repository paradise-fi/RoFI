#pragma once

#include <atomic>
#include <cassert>
#include <functional>
#include <map>
#include <stop_token>
#include <thread>
#include <type_traits>

#include <atoms/concurrent_queue.hpp>

#include <rofi_hal.hpp>

#include <rofiResp.pb.h>


namespace rofi::hal
{
class WaitCallbacks {
public:
    using Callback = std::function< void() >;

    /**
     * \brief Stores a callback under given wait id
     *
     * \param waitId A unique identifier of the expected wait response
     * \param callback Callback to store
     *
     * \retval `true` when the insertion was successful
     * \retval `false` when there exists a callback with given \p waitId
     */
    bool registerWaitCallback( int waitId, Callback && callback )
    {
        return _callbacks.try_emplace( waitId, std::move( callback ) ).second;
    }

    Callback getAndEraseCallback( int waitId )
    {
        auto oldCallback = std::move( _callbacks[ waitId ] );
        _callbacks.erase( waitId );
        return oldCallback;
    }

private:
    std::map< int, Callback > _callbacks;
};


class WaitWorker {
    using Message = rofi::messages::RofiResp;
    using WaitCallback = WaitCallbacks::Callback;

public:
    WaitWorker()
    {
        _workerThread = std::jthread(
                [ this ]( std::stop_token stoken ) { this->run( std::move( stoken ) ); } );
    }

    WaitWorker( const WaitWorker & ) = delete;
    WaitWorker & operator=( const WaitWorker & ) = delete;

    ~WaitWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() ) {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }


    void processMessage( int waitId )
    {
        _waitIdsQueue.push( waitId );
    }

    // Returns the wait Id
    int registerWaitCallback( WaitCallback && callback )
    {
        assert( callback );

        auto waitId = _nextWaitId.fetch_add( 1 );

        [[maybe_unused]] auto success = _callbacks->registerWaitCallback( waitId,
                                                                          std::move( callback ) );
        assert( success );

        return waitId;
    }

private:
    void callCallback( int waitId )
    {
        auto callback = _callbacks->getAndEraseCallback( waitId );
        if ( callback ) {
            callback();
        } else {
            std::cerr << "Got wait response without a callback waiting (ID: " << waitId
                      << "). Ignoring...\n";
        }
    }

    void run( std::stop_token stoken )
    {
        while ( true ) {
            auto waitId = _waitIdsQueue.pop( stoken );
            if ( !waitId ) {
                return;
            }
            callCallback( *waitId );
        }
    }


    atoms::Guarded< WaitCallbacks > _callbacks;
    atoms::ConcurrentQueue< int > _waitIdsQueue;
    std::atomic_int _nextWaitId = 1;

    std::jthread _workerThread;
};

} // namespace rofi::hal
