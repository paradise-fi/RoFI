#include "rofi_hal_sim.hpp"

#include <cassert>

namespace rofi
{
    namespace hal
    {
        RoFI::RoFI() : rofiData( std::make_unique< detail::RoFIData >( 3, 6 ) ) {}

        RoFI::~RoFI() = default;

        RoFI & RoFI::getLocalRoFI()
        {
            static RoFI localRoFI;
            return localRoFI;
        }

        Joint RoFI::getJoint( int index )
        {
            return rofiData->getJoint( index );
        }

        Connector RoFI::getConnector( int index )
        {
            return rofiData->getConnector( index );
        }


        Joint::Joint( detail::JointData & jdata ) : jointData( & jdata ) {}

        float Joint::maxPosition() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_POSITION );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_POSITION );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float Joint::minPosition() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_MIN_POSITION );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MIN_POSITION );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float Joint::maxSpeed() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_SPEED );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_SPEED );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float Joint::minSpeed() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_MIN_SPEED );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MIN_SPEED );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float Joint::maxTorque() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_MAX_TORQUE );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_MAX_TORQUE );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        float Joint::getVelocity() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_VELOCITY );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_VELOCITY );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void Joint::setVelocity( float velocity )
        {
            auto msg = jointData->getCmdMsg( messages::JointCmd::SET_VELOCITY );
            msg.mutable_jointcmd()->mutable_setvelocity()->set_velocity( velocity );
            jointData->rofi.pub->Publish( std::move( msg ), true );
        }

        float Joint::getPosition() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_POSITION );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_POSITION );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void Joint::setPosition( float pos, float speed, std::function< void( Joint ) > callback )
        {
            if ( callback )
            {
                jointData->registerCallback( [ pos ]( const messages::JointResp & resp ){
                            if ( resp.cmdtype() != messages::JointCmd::SET_POS_WITH_SPEED )
                                return false;
                            if ( resp.values_size() != 1 )
                                return false;
                            return detail::equal( resp.values().Get( 0 ), pos );
                            },
                        std::move( callback ) );
            }

            auto msg = jointData->getCmdMsg( messages::JointCmd::SET_POS_WITH_SPEED );
            msg.mutable_jointcmd()->mutable_setposwithspeed()->set_position( pos );
            msg.mutable_jointcmd()->mutable_setposwithspeed()->set_speed( speed );
            jointData->rofi.pub->Publish( std::move( msg ), true );
        }

        float Joint::getTorque() const
        {
            auto result = jointData->registerPromise( messages::JointCmd::GET_TORQUE );
            auto msg = jointData->getCmdMsg( messages::JointCmd::GET_TORQUE );
            jointData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            if ( resp->jointresp().values_size() != 1 )
                throw std::runtime_error( "Unexpected size of values from response" );
            return resp->jointresp().values().Get( 0 );
        }

        void Joint::setTorque( float torque )
        {
            auto msg = jointData->getCmdMsg( messages::JointCmd::SET_TORQUE );
            msg.mutable_jointcmd()->mutable_settorque()->set_torque( torque );
            jointData->rofi.pub->Publish( std::move( msg ), true );
        }


        Connector::Connector( detail::ConnectorData & cdata ) : connectorData( & cdata ) {}

        ConnectorState Connector::getState() const
        {
            auto result = connectorData->registerPromise( messages::ConnectorCmd::GET_STATE );
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::GET_STATE );
            connectorData->rofi.pub->Publish( std::move( msg ), true );

            auto resp = result.get();
            auto respState = resp->connectorresp().state();
            return {
                static_cast< ConnectorPosition >( respState.position() ),
                respState.internal(),
                respState.external(),
                respState.connected(),
                static_cast< ConnectorOrientation >( respState.orientation() )
            };
        }

        void Connector::connect()
        {
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::CONNECT );
            connectorData->rofi.pub->Publish( std::move( msg ), true );
        }

        void Connector::disconnect()
        {
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::DISCONNECT );
            connectorData->rofi.pub->Publish( std::move( msg ), true );
        }

        void Connector::onPacket( std::function< void ( Connector, Packet ) > callback )
        {
            if ( !callback )
            {
                return;
            }
            connectorData->registerCallback( []( const messages::ConnectorResp & resp ){
                        return resp.cmdtype() == messages::ConnectorCmd::PACKET;
                        },
                    [ callback = std::move( callback ) ]( Connector connector, detail::RoFIData::RofiRespPtr resp ){
                        callback( connector, detail::ConnectorData::getPacket( std::move( resp ) ) );
                        } );
        }

        void Connector::send( Packet packet )
        {
            static_assert( sizeof( Packet::value_type ) == sizeof( char ) );
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::PACKET );
            msg.mutable_connectorcmd()->mutable_packet()->set_message( packet.data(), packet.size() );
            connectorData->rofi.pub->Publish( std::move( msg ), true );
        }

        void Connector::connectPower( ConnectorLine )
        {
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::CONNECT_POWER );
            connectorData->rofi.pub->Publish( std::move( msg ), true );
        }

        void Connector::disconnectPower( ConnectorLine )
        {
            auto msg = connectorData->getCmdMsg( messages::ConnectorCmd::DISCONNECT_POWER );
            connectorData->rofi.pub->Publish( std::move( msg ), true );
        }

        namespace detail
        {
            RoFIData::RoFIData( int jointNumber, int connectorNumber )
            {
                for ( int i = 0; i < jointNumber; i++ )
                {
                    joints.push_back( std::make_unique< JointData >( *this, i ) );
                }

                for ( int i = 0; i < connectorNumber; i++ )
                {
                    connectors.push_back( std::make_unique< ConnectorData >( *this, i ) );
                }

                node = boost::make_shared< gazebo::transport::Node >();
                node->Init();

                std::string moduleName = "universalModule";

                sub = node->Subscribe( "~/" + moduleName + "/response", & RoFIData::onResponse, this );
                pub = node->Advertise< rofi::messages::RofiCmd >( "~/" + moduleName + "/control" );

                std::cerr << "Waiting for connection...\n";
                pub->WaitForConnection();
                std::cerr << "Connected\n";
            }

            Joint RoFIData::getJoint( int index )
            {
                if ( index < 0 || static_cast< size_t >( index ) > joints.size() )
                {
                    throw std::out_of_range( "Joint index is out of range" );
                }
                return joints[ index ]->getJoint();
            }

            Connector RoFIData::getConnector( int index )
            {
                if ( index < 0 || static_cast< size_t >( index ) > connectors.size() )
                {
                    throw std::out_of_range( "Connector index is out of range" );
                }
                return connectors[ index ]->getConnector();
            }

            void RoFIData::onResponse( const RoFIData::RofiRespPtr & resp )
            {
                using rofi::messages::RofiCmd;

                RofiCmd::Type resptype = resp->resptype();
                switch ( resptype )
                {
                    case RofiCmd::JOINT_CMD:
                    {
                        int index = resp->jointresp().joint();
                        if ( index < 0 || static_cast< size_t >( index ) > joints.size() )
                        {
                            std::cerr << "Joint index of response is out of range\nIgnoring...\n";
                            return;
                        }
                        joints[ index ]->onResponse( resp );
                        break;
                    }
                    case RofiCmd::CONNECTOR_CMD:
                    {
                        int index = resp->connectorresp().connector();
                        if ( index < 0 || static_cast< size_t >( index ) > connectors.size() )
                        {
                            std::cerr << "Connector index of response is out of range\nIgnoring...\n";
                            return;
                        }
                        connectors[ index ]->onResponse( resp );
                        break;
                    }
                    default:
                    {
                        std::cerr << "Not recognized rofi cmd type\nIgnoring...\n";
                        break;
                    }
                }
            }


            Joint JointData::getJoint()
            {
                return Joint( *this );
            }

            messages::RofiCmd JointData::getCmdMsg( messages::JointCmd::Type type ) const
            {
                messages::RofiCmd rofiCmd;
                rofiCmd.set_cmdtype( messages::RofiCmd::JOINT_CMD );
                rofiCmd.mutable_jointcmd()->set_joint( jointNumber );
                rofiCmd.mutable_jointcmd()->set_cmdtype( type );
                return rofiCmd;
            }

            std::future< RoFIData::RofiRespPtr > JointData::registerPromise( messages::JointCmd::Type type )
            {
                std::lock_guard< std::mutex > lock( respMapMutex );
                return respMap.emplace( type, std::promise< RoFIData::RofiRespPtr >() )->second.get_future();
            }

            void JointData::registerCallback( Check && pred, Callback && callback )
            {
                Callback oldCallback;
                {
                    std::lock_guard< std::mutex > lock( respCallbackMutex );
                    oldCallback = std::move( respCallback.second );
                    respCallback = { std::move( pred ), std::move( callback ) };
                }
                if ( oldCallback )
                {
                    std::cerr << "Aborting old callback\n";
                    // TODO abort oldCallback
                }
            }

            void JointData::onResponse( const RoFIData::RofiRespPtr & resp )
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
                }
                {
                    std::lock_guard< std::mutex > lock( respCallbackMutex );
                    auto & check = respCallback.first;
                    if ( check && check( resp->jointresp() ) )
                    {
                        if ( respCallback.second )
                        {
                            std::thread( std::move( respCallback.second ), getJoint() ).detach();
                        }
                        respCallback = {};
                    }
                }
            }


            Connector ConnectorData::getConnector()
            {
                return Connector( *this );
            }

            messages::RofiCmd ConnectorData::getCmdMsg( messages::ConnectorCmd::Type type ) const
            {
                messages::RofiCmd rofiCmd;
                rofiCmd.set_cmdtype( messages::RofiCmd::CONNECTOR_CMD );
                rofiCmd.mutable_connectorcmd()->set_connector( connectorNumber );
                rofiCmd.mutable_connectorcmd()->set_cmdtype( type );
                return rofiCmd;
            }

            Connector::Packet ConnectorData::getPacket( RoFIData::RofiRespPtr resp )
            {
                static_assert( sizeof( Connector::Packet::value_type ) == sizeof( char ) );

                const auto & message = resp->connectorresp().packet().message();
                Connector::Packet packet;
                packet.reserve( message.size() );
                for ( auto byte : message )
                {
                    packet.push_back( static_cast< Connector::Packet::value_type >( byte ) );
                }
                return packet;
            }

            std::future< RoFIData::RofiRespPtr > ConnectorData::registerPromise( messages::ConnectorCmd::Type type )
            {
                std::lock_guard< std::mutex > lock( respMapMutex );
                return respMap.emplace( type, std::promise< RoFIData::RofiRespPtr >() )->second.get_future();
            }

            void ConnectorData::registerCallback( Check && pred, Callback && callback )
            {
                std::lock_guard< std::mutex > lock( respCallbacksMutex );
                respCallbacks.emplace_back( std::move( pred ), std::move( callback ) );
            }

            void ConnectorData::onResponse( const RoFIData::RofiRespPtr & resp )
            {
                assert( resp->resptype() == messages::RofiCmd::CONNECTOR_CMD );
                assert( resp->connectorresp().connector() == connectorNumber );

                {
                    std::lock_guard< std::mutex > lock( respMapMutex );
                    auto range = respMap.equal_range( resp->connectorresp().cmdtype() );
                    for ( auto it = range.first; it != range.second; it++ )
                    {
                        it->second.set_value( resp );
                    }
                    respMap.erase( range.first, range.second );
                }
                {
                    std::lock_guard< std::mutex > lock( respCallbacksMutex );
                    for ( const auto & respCallback : respCallbacks )
                    {
                        auto & check = respCallback.first;
                        if ( check && check( resp->connectorresp() ) )
                        {
                            if ( respCallback.second )
                            {
                                std::thread( respCallback.second, getConnector(), resp ).detach();
                            }
                        }
                    }
                }
            }
        } // namespace detail
    } // namespace hal
} // namespace rofi
