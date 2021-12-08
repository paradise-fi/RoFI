#pragma once

#include <cassert>
#include <functional>
#include <stop_token>
#include <thread>
#include <type_traits>

#include "callbacks.hpp"
#include "atoms/concurrent_queue.hpp"
#include "rofi_hal.hpp"

#include <jointResp.pb.h>


namespace rofi::hal
{
class PositionCallbackHandle
{
public:
    using PositionCallback = std::function< void( Joint ) >;

    static constexpr float positionPrecision = 1e-3f;

    PositionCallbackHandle() = default;

    PositionCallbackHandle( const PositionCallbackHandle & ) = delete;
    PositionCallbackHandle & operator=( const PositionCallbackHandle & ) = delete;
    PositionCallbackHandle & operator=( PositionCallbackHandle && ) = delete;

    PositionCallbackHandle( PositionCallbackHandle && other )
    {
        std::lock_guard< std::mutex > lock( other._positionCallbackMutex );
        _positionCallback = std::move( other._positionCallback );
        _desiredPosition = std::move( other._desiredPosition );
    }


    // Returns old callback
    PositionCallback setPositionCallback( float desiredPosition,
                                          PositionCallback && positionCallback )
    {
        std::lock_guard< std::mutex > lock( _positionCallbackMutex );
        auto oldCallback = std::move( _positionCallback );
        _positionCallback = std::move( positionCallback );
        _desiredPosition = desiredPosition;

        return oldCallback;
    }

    PositionCallback getAndClearPositionCallback( float reachedPosition )
    {
        std::lock_guard< std::mutex > lock( _positionCallbackMutex );

        if ( !checkPositionReached( _desiredPosition, reachedPosition ) )
        {
            return {};
        }

        auto oldCallback = std::move( _positionCallback );
        _positionCallback = {};

        return oldCallback;
    }

private:
    static bool checkPositionReached( float desiredPosition, float reachedPosition )
    {
        return std::abs( reachedPosition - desiredPosition ) <= positionPrecision;
    }

    mutable std::mutex _positionCallbackMutex;
    PositionCallback _positionCallback;
    float _desiredPosition = {};
};


class JointWorker
{
    using Message = rofi::messages::JointResp;
    using PositionCallback = PositionCallbackHandle::PositionCallback;
    using ErrorCallback = std::function< void( Joint, Joint::Error, const std::string & ) >;

    struct JointCallbacks
    {
        Callbacks< ErrorCallback > errorCallbacks;
        PositionCallbackHandle positionCallbackHandle;
    };

public:
    JointWorker() = default;

    JointWorker( const JointWorker & ) = delete;
    JointWorker & operator=( const JointWorker & ) = delete;

    ~JointWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() )
        {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }

    void init( std::weak_ptr< RoFI::Implementation > rofi, int jointCount )
    {
        assert( jointCount >= 0 );
        assert( rofi.lock() );

        _rofi = std::move( rofi );
        _callbacks.resize( jointCount );

        _workerThread = std::jthread(
                [ this ]( std::stop_token stoken ) { this->run( std::move( stoken ) ); } );
    }


    void processMessage( const Message & msg )
    {
        _queue.push( msg );
    }

    void registerErrorCallback( int jointIndex, ErrorCallback && callback )
    {
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );
        assert( callback );

        _callbacks[ jointIndex ].errorCallbacks.registerCallback( std::move( callback ) );
    }

    void registerPositionCallback( int jointIndex,
                                   float desiredPosition,
                                   PositionCallback && callback )
    {
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );
        assert( callback );

        auto oldCallback = _callbacks[ jointIndex ].positionCallbackHandle.setPositionCallback(
                desiredPosition,
                std::move( callback ) );
        if ( oldCallback )
        {
            std::cerr << "Aborting old set position callback\n";
            // TODO abort old callback
        }
    }

private:
    std::shared_ptr< RoFI::Implementation > getRoFI() const
    {
        auto rofi = _rofi.lock();
        assert( rofi && "RoFI invalid access from joint worker" );
        return rofi;
    }

    Joint getJoint( int index ) const
    {
        auto rofi = getRoFI();

        assert( static_cast< size_t >( rofi->getDescriptor().jointCount ) == _callbacks.size() );
        assert( index >= 0 );
        assert( index < rofi->getDescriptor().jointCount );

        return rofi->getJoint( index );
    }

    static std::pair< Joint::Error, std::string > readError(
            const rofi::messages::JointResp & /* jointResp */ )
    {
        assert( false && "not implemented" );
        throw std::runtime_error( "readError not implemented" );
    }

    void callCallbacks( const Message & message )
    {
        auto jointIndex = message.joint();
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );

        auto joint = getJoint( jointIndex );
        switch ( message.resptype() )
        {
            case rofi::messages::JointCmd::ERROR:
            {
                auto [ errorType, errorStr ] = readError( message );
                auto & errCallbacks = _callbacks[ jointIndex ].errorCallbacks;
                errCallbacks.callCallbacks( std::move( joint ), errorType, std::move( errorStr ) );
                break;
            }
            case rofi::messages::JointCmd::SET_POS_WITH_SPEED:
            {
                float reachedPosition = message.value();
                auto & posCallbackHandle = _callbacks[ jointIndex ].positionCallbackHandle;
                auto callback = posCallbackHandle.getAndClearPositionCallback( reachedPosition );

                callback( std::move( joint ) );
                break;
            }
            default:
                assert( false );
        }
    }

    void run( std::stop_token stoken )
    {
        while ( true )
        {
            auto message = _queue.pop( stoken );
            if ( !message )
            {
                return;
            }
            callCallbacks( *message );
        }
    }


    std::weak_ptr< RoFI::Implementation > _rofi;

    std::vector< JointCallbacks > _callbacks;
    atoms::ConcurrentQueue< Message > _queue;

    std::jthread _workerThread;
};

} // namespace rofi::hal
