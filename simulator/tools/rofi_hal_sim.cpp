#include "rofi_hal_sim.hpp"

#include <cassert>

namespace rofi
{
    namespace hal
    {
        RoFI::RoFI() : rdata( boost::make_shared< Data >( 3 ) ) {}

        RoFI::Joint RoFI::getJoint( int index )
        {
            if ( index < 0 || static_cast< size_t >( index ) > rdata->joints.size() )
                throw std::out_of_range( "Joint index is out of range" );
            return rdata->joints[ index ]->getJoint();
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
            pub = node->Advertise< rofi::messages::RofiCmd >( "~/" + moduleName + "/control" );

            std::cerr << "Waiting for connection...\n";
            pub->WaitForConnection();
            std::cerr << "Connected\n";
        }

        void RoFI::Data::onResponse( const RoFI::Data::RofiRespPtr & resp )
        {
            std::cerr << "Got response\n";
            messages::RofiCmd::Type resptype = resp->resptype();
            switch ( resptype )
            {
                case messages::RofiCmd::JOINT_CMD:
                {
                    int index = resp->jointresp().joint();
                    if ( index < 0 || static_cast< size_t >( index ) > joints.size() )
                        throw std::out_of_range( "Joint index is out of range" );
                    joints[ index ]->onResponse( resp );
                    break;
                }
                default:
                    // TODO
                    break;
            }
        }

        RoFI::Joint RoFI::Joint::Data::getJoint()
        {
            return Joint( *this );
        }

        messages::RofiCmd RoFI::Joint::Data::getCmdMsg( messages::JointCmd::Type type ) const
        {
            messages::RofiCmd rofiCmd;
            rofiCmd.set_cmdtype( messages::RofiCmd::JOINT_CMD );
            rofiCmd.mutable_jointcmd()->set_joint( jointNumber );
            rofiCmd.mutable_jointcmd()->set_cmdtype( type );
            return rofiCmd;
        }

        std::future< RoFI::Data::RofiRespPtr > RoFI::Joint::Data::registerPromise( messages::JointCmd::Type type )
        {
            std::lock_guard< std::mutex > lock( respMapMutex );
            return respMap.emplace( type, std::promise< RoFI::Data::RofiRespPtr >() )->second.get_future();
        }

        void RoFI::Joint::Data::registerCallback( std::function< bool( const messages::JointResp & ) > pred, std::function< void( Joint ) > callback )
        {
            std::lock_guard< std::mutex > lock( respCallbacksMutex );
            respCallbacks.emplace_back( pred, callback );
        }

        void RoFI::Joint::Data::onResponse( const RoFI::Data::RofiRespPtr & resp )
        {
            assert( resp->resptype() == messages::RofiCmd::JOINT_CMD );
            assert( resp->jointresp().joint() == jointNumber );

            {
                std::lock_guard< std::mutex > lock( respMapMutex );
                auto range = respMap.equal_range( resp->jointresp().cmdtype() );
                for ( auto it = range.first; it != range.second; it++ )
                {
                    it->second.set_value( resp );
                }
                respMap.erase( range.first, range.second );
                if ( range.first == range.second )
                    std::cerr << "No promises for response\n";
            }
            {
                std::lock_guard< std::mutex > lock( respCallbacksMutex );
                for ( auto callback : respCallbacks )
                {
                    if ( callback.first( resp->jointresp() ) )
                        std::thread( callback.second, getJoint() ).detach();
                }
            }
        }


        float RoFI::Joint::maxPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float RoFI::Joint::minPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MIN_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float RoFI::Joint::maxSpeed() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_SPEED );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float RoFI::Joint::minSpeed() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MIN_SPEED );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float RoFI::Joint::maxTorque() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_MAX_TORQUE );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float RoFI::Joint::getVelocity() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_VELOCITY );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_VELOCITY );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void RoFI::Joint::setVelocity( float velocity )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_VELOCITY );
            msg.mutable_jointcmd()->mutable_setvelocity()->set_velocity( velocity );
            jdata->rofi.pub->Publish( msg, true );
        }

        float RoFI::Joint::getPosition() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_POSITION );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void RoFI::Joint::setPosition( float pos, float speed, void ( *callback )( RoFI::Joint ) )
        {
            if ( callback )
            {
                jdata->registerCallback( [ pos ]( const messages::JointResp & resp ){
                            if ( resp.cmdtype() != messages::JointCmd::SET_POS_WITH_SPEED )
                                return false;
                            if ( resp.values_size() != 1 )
                                return false;
                            return resp.values().Get( 0 ) == pos;
                            },
                        callback );
            }

            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_POS_WITH_SPEED );
            msg.mutable_jointcmd()->mutable_setposwithspeed()->set_position( pos );
            msg.mutable_jointcmd()->mutable_setposwithspeed()->set_speed( speed );
            jdata->rofi.pub->Publish( msg, true );
        }

        float RoFI::Joint::getTorque() const
        {
            auto result = jdata->registerPromise( messages::JointCmd::GET_TORQUE );
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void RoFI::Joint::setTorque( float torque )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_TORQUE );
            msg.mutable_jointcmd()->mutable_settorque()->set_torque( torque );
            jdata->rofi.pub->Publish( msg, true );
        }
    }
}
