#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include <gazebo/transport/transport.hh>

#include "rofi_hal.hpp"

#include <jointCmd.pb.h>
#include <jointResp.pb.h>

namespace rofi
{
    namespace hal
    {
        class RoFI::Data
        {
        public:
            std::vector< std::unique_ptr< Joint::Data > > joints;
            gazebo::transport::NodePtr node;
            gazebo::transport::PublisherPtr pub;
            gazebo::transport::SubscriberPtr sub;

            Data( int jointNumber );

            void onResponse( const boost::shared_ptr< const messages::JointResp > & resp );
        };

        class RoFI::Joint::Data
        {
        public:
            using JointRespPtr = boost::shared_ptr< const messages::JointResp >;
            friend class RoFI::Data;

        private:
            std::unordered_multimap< messages::JointCmd::Type, std::promise< JointRespPtr > > respMap;
            std::mutex respMapMutex;

            std::vector< std::pair< std::function< bool( JointRespPtr ) >, std::function< void( Joint ) > > > respCallbacks;
            std::mutex respCallbacksMutex;

        public:
            Data( RoFI::Data & rofi, int jointNumber ) : rofi( rofi ), jointNumber( jointNumber ) {}

            RoFI::Data &rofi;
            int jointNumber;

            Joint getJoint();

            messages::JointCmd getCmdMsg( messages::JointCmd::Type type ) const;
            std::future< JointRespPtr > registerPromise( messages::JointCmd::Type type );

            void registerCallback( std::function< bool( JointRespPtr ) > pred, std::function< void( Joint ) > callback );
        };
    }
}
