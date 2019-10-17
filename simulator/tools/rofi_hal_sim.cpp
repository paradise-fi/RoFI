#include "rofi_hal_sim.hpp"

#include <jointCmd.pb.h>

namespace rofi
{
    namespace hal
    {
        RoFI::RoFI() : rdata( std::make_shared< Data >( 3 ) ) {}

        RoFI::Joint RoFI::getJoint( int index )
        {
            return rdata->joints.at( index );
        }

        RoFI::Data::Data( int jointNumber )
        {
            for ( int i = 0; i < jointNumber; i++ )
            {
                Joint joint;
                joint.jdata = std::make_shared< Joint::Data >( *this, i );
                joints.push_back( joint );
            }

            node = gazebo::transport::NodePtr( new gazebo::transport::Node() );
            node->Init();

            pub = node->Advertise< rofi::messages::JointCmd >( "~/universalModule/control" );

            std::cerr << "Waiting for connection...\n";
            pub->WaitForConnection();
            std::cerr << "Connected\n";
        }

        messages::JointCmd RoFI::Joint::Data::getCmdMsg( messages::JointCmd::Type type )
        {
            messages::JointCmd jointCmd;
            jointCmd.set_joint( jointNumber );
            jointCmd.set_cmdtype( type );
            return jointCmd;
        }

        float RoFI::Joint::maxPosition() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        float RoFI::Joint::minPosition() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        float RoFI::Joint::maxSpeed() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        float RoFI::Joint::minSpeed() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MIN_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        float RoFI::Joint::maxTorque() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_MAX_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        float RoFI::Joint::getSpeed() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_SPEED );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        void RoFI::Joint::setSpeed( float speed )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_SPEED );
            msg.mutable_setspeed()->set_speed( speed );
            jdata->rofi.pub->Publish( msg );
        }

        float RoFI::Joint::getPosition() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_POSITION );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        void RoFI::Joint::setPosition( float pos, float speed, void (*callback)( Joint ))
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_POS_WITH_SPEED );
            msg.mutable_setposwithspeed()->set_position( pos );
            msg.mutable_setposwithspeed()->set_speed( speed );
            jdata->rofi.pub->Publish( msg );

            // TODO Callback
        }

        float RoFI::Joint::getTorque() const
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::GET_TORQUE );
            jdata->rofi.pub->Publish( msg, true );

            // TODO receive
            return 0;
        }

        void RoFI::Joint::setTorque( float torque )
        {
            auto msg = jdata->getCmdMsg( messages::JointCmd::SET_TORQUE );
            msg.mutable_settorque()->set_torque( torque );
            jdata->rofi.pub->Publish( msg );
        }
    }
}
