#include "rofi_hal_sim.hpp"

#include <jointCmd.pb.h>

namespace rofi
{
    namespace hal
    {
        RoFI::RoFI() : rdata( boost::make_shared< Data >( 3 ) ) {}

        RoFI::Joint RoFI::getJoint( int index )
        {
            return rdata->joints.at( index )->getJoint();
        }

        RoFI::Joint::Joint( RoFI::Joint::Data & data ) : jdata( &data ) {}

        RoFI::Data::Data( int jointNumber )
        {
            for ( int i = 0; i < jointNumber; i++ )
            {
                joints.push_back( std::make_unique< Joint::Data >( *this, i ) );
            }

            node = gazebo::transport::NodePtr( new gazebo::transport::Node() );
            node->Init();

            std::string moduleName = "universalModule";

            sub = node->Subscribe( "~/" + moduleName + "/response", &RoFI::Data::onResponse, this );
            pub = node->Advertise< rofi::messages::JointCmd >( "~/" + moduleName + "/control" );

            std::cerr << "Waiting for connection...\n";
            pub->WaitForConnection();
            std::cerr << "Connected\n";
        }

        void RoFI::Data::onResponse( const boost::shared_ptr< const messages::JointResp > & resp )
        {
            std::cerr << "Got response\n";

            int respJoint = resp->joint();
            if ( respJoint < 0 || static_cast< size_t >( respJoint ) >= joints.size() )
            {
                std::cerr << "Joint response has invalid joint: " << respJoint << "\n";
                return;
            }
            auto & jdata = joints[ respJoint ];
            {
                std::lock_guard< std::mutex > lg( jdata->respMapMutex );
                auto range = jdata->respMap.equal_range( resp->cmdtype() );
                for ( auto it = range.first; it != range.second; it++ )
                {
                    it->second.set_value( resp );
                }
                if ( range.first == range.second )
                    std::cerr << "No waiting futures for promise\n";
            }
            {
                std::lock_guard< std::mutex > lock( jdata->respCallbacksMutex );
                for ( auto callback : jdata->respCallbacks )
                {
                    if ( callback.first( resp ) )
                        std::thread( callback.second, jdata->getJoint() ).detach();
                }
            }
        }

        RoFI::Joint RoFI::Joint::Data::getJoint()
        {
            return Joint( *this );
        }

        messages::JointCmd RoFI::Joint::Data::getCmdMsg( messages::JointCmd::Type type ) const
        {
            messages::JointCmd jointCmd;
            jointCmd.set_joint( jointNumber );
            jointCmd.set_cmdtype( type );
            return jointCmd;
        }

        std::future< RoFI::Joint::Data::JointRespPtr >
                RoFI::Joint::Data::registerPromise( messages::JointCmd::Type type )
        {
            std::lock_guard< std::mutex > lg( respMapMutex );
            return respMap.emplace( type, std::promise< JointRespPtr >() )->second.get_future();
        }

        void RoFI::Joint::Data::registerCallback( std::function< bool( JointRespPtr ) > pred, std::function< void( Joint ) > callback )
        {
            std::lock_guard< std::mutex > lock( respCallbacksMutex );
            respCallbacks.emplace_back( pred, callback );
        }


        float RoFI::Joint::maxPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        float RoFI::Joint::minPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MIN_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        float RoFI::Joint::maxSpeed() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_SPEED );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        float RoFI::Joint::minSpeed() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MIN_SPEED );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        float RoFI::Joint::maxTorque() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_TORQUE );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        float RoFI::Joint::getSpeed() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_SPEED );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        void RoFI::Joint::setSpeed( float speed )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_SPEED );
            msg.mutable_setspeed()->set_speed( speed );
            jdata->rofi.pub->Publish( msg, true );
        }

        float RoFI::Joint::getPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        void RoFI::Joint::setPosition( float pos, float speed, void ( *callback )( RoFI::Joint ) )
        {
            if ( callback )
            {
                jdata->registerCallback( [ pos ]( Data::JointRespPtr resp ){
                            if ( resp->cmdtype() != messages::JointCmd::SET_POS_WITH_SPEED )
                                return false;
                            if ( resp->values_size() != 1 )
                                return false;
                            return resp->values().Get( 0 ) == pos;
                            },
                        callback );
            }

            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_POS_WITH_SPEED );
            msg.mutable_setposwithspeed()->set_position( pos );
            msg.mutable_setposwithspeed()->set_speed( speed );
            jdata->rofi.pub->Publish( msg, true );
        }

        float RoFI::Joint::getTorque() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_TORQUE );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->values().Get( 0 );
        }

        void RoFI::Joint::setTorque( float torque )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_TORQUE );
            msg.mutable_settorque()->set_torque( torque );
            jdata->rofi.pub->Publish( msg, true );
        }
    }
}
