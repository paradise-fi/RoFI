#include <map>

#include <gazebo/transport/transport.hh>

#include "rofi_hal.hpp"

#include <jointCmd.pb.h>

namespace rofi
{
    namespace hal
    {
        class RoFI::Data
        {
        public:
            std::vector< Joint > joints;
            gazebo::transport::NodePtr node;
            gazebo::transport::PublisherPtr pub;

            Data( int jointNumber );
        };

        class RoFI::Joint::Data
        {
        public:
            Data( RoFI::Data & rofi, int jointNumber ) : rofi( rofi ), jointNumber( jointNumber ) {}

            RoFI::Data &rofi;
            int jointNumber;

            messages::JointCmd getCmdMsg( messages::JointCmd::Type type );
        };
    }
}
