#pragma once

#include <cassert>
#include <functional>
#include <stop_token>
#include <thread>
#include <type_traits>

#include <atoms/concurrent_queue.hpp>
#include <atoms/guarded.hpp>

#include <rofi_hal.hpp>

#include <jointResp.pb.h>


namespace rofi::hal
{
class PositionCallbackHandle {
public:
    using PositionCallback = std::function< void( Joint ) >;

    static constexpr float positionPrecision = 1e-3f;


    // Returns old callback
    PositionCallback setPositionCallback( float desiredPosition,
                                          PositionCallback && positionCallback )
    {
        assert( positionCallback );

        auto oldCallback = std::move( _positionCallback );
        _positionCallback = std::move( positionCallback );
        _desiredPosition = desiredPosition;

        return oldCallback;
    }

    // Returns old callback
    PositionCallback clearPositionCallback()
    {
        auto oldCallback = std::move( _positionCallback );
        _positionCallback = {};
        return oldCallback;
    }

    PositionCallback getAndClearPositionCallback( float reachedPosition )
    {
        if ( std::abs( reachedPosition - _desiredPosition ) > positionPrecision ) {
            return {};
        }

        auto oldCallback = std::move( _positionCallback );
        _positionCallback = {};

        return oldCallback;
    }

private:
    PositionCallback _positionCallback;
    float _desiredPosition = {};
};


class JointWorker {
    using Message = rofi::messages::JointResp;
    using PositionCallback = PositionCallbackHandle::PositionCallback;
    using ErrorCallback = std::function< void( Joint, Joint::Error, const std::string & ) >;

    struct JointCallbacks {
        atoms::Guarded< ErrorCallback > errorCallback;
        atoms::Guarded< PositionCallbackHandle > positionCallbackHandle;
    };

public:
    JointWorker() = default;

    JointWorker( const JointWorker & ) = delete;
    JointWorker & operator=( const JointWorker & ) = delete;

    ~JointWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() ) {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }

    void init( std::weak_ptr< RoFI::Implementation > rofi, int jointCount )
    {
        assert( jointCount >= 0 );
        assert( !rofi.expired() );

        assert( _rofi.expired() );
        assert( _callbacks.empty() );
        assert( !_workerThread.joinable() );

        _rofi = std::move( rofi );
        // resize doesn't work because `std::mutex` isn't noexcept move constructible
        _callbacks = std::vector< JointCallbacks >( jointCount );

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

        _callbacks[ jointIndex ].errorCallback.replace( std::move( callback ) );
    }

    void registerPositionCallback( int jointIndex,
                                   float desiredPosition,
                                   PositionCallback && callback )
    {
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );
        assert( callback );

        auto oldCallback = _callbacks[ jointIndex ]
                                   .positionCallbackHandle
                                   ->setPositionCallback( desiredPosition, std::move( callback ) );
        if ( oldCallback ) {
            onAbortPositionCallback( std::move( oldCallback ) );
        }
    }

    void abortPositionCallback( int jointIndex )
    {
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );

        auto oldCallback = _callbacks[ jointIndex ].positionCallbackHandle->clearPositionCallback();
        if ( oldCallback ) {
            onAbortPositionCallback( std::move( oldCallback ) );
        }
    }

private:
    static void onAbortPositionCallback( [[maybe_unused]] PositionCallback callback )
    {
        std::cerr << "Aborting old set position callback\n";
        // TODO abort old callback
    }

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

    void callCallback( const Message & message )
    {
        auto jointIndex = message.joint();
        assert( jointIndex >= 0 );
        assert( static_cast< size_t >( jointIndex ) < _callbacks.size() );

        auto joint = getJoint( jointIndex );
        switch ( message.resptype() ) {
            case rofi::messages::JointCmd::ERROR:
            {
                auto [ errorType, errorStr ] = readError( message );
                _callbacks[ jointIndex ].errorCallback.visit( [ & ]( auto & callback ) {
                    if ( callback ) {
                        std::invoke( callback,
                                     std::move( joint ),
                                     errorType,
                                     std::move( errorStr ) );
                    }
                } );
                return;
            }
            case rofi::messages::JointCmd::SET_POS_WITH_SPEED:
            {
                float reachedPosition = message.value();
                auto & posCallbackHandle = _callbacks[ jointIndex ].positionCallbackHandle;
                auto callback = posCallbackHandle->getAndClearPositionCallback( reachedPosition );

                if ( callback ) {
                    callback( std::move( joint ) );
                }
                return;
            }
            default:
                assert( false );
                return;
        }
    }

    void run( std::stop_token stoken )
    {
        while ( true ) {
            auto message = _queue.pop( stoken );
            if ( !message ) {
                return;
            }
            callCallback( *message );
        }
    }


    std::weak_ptr< RoFI::Implementation > _rofi;

    std::vector< JointCallbacks > _callbacks;
    atoms::ConcurrentQueue< Message > _queue;

    std::jthread _workerThread;
};

} // namespace rofi::hal
