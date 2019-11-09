#include <functional>
#include <future>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include <gazebo/transport/transport.hh>

#include "rofi_hal.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

namespace rofi
{
    namespace hal
    {
        constexpr double doublePrecision = 0.01;

        inline bool equal( double first, double second, double precision = doublePrecision )
        {
            return first <= second + precision && second <= first + precision;
        }

        class RoFI::Data
        {
        public:
            using RofiRespPtr = boost::shared_ptr< const messages::RofiResp >;

            std::vector< std::unique_ptr< Joint::Data > > joints;
            gazebo::transport::NodePtr node;
            gazebo::transport::PublisherPtr pub;
            gazebo::transport::SubscriberPtr sub;

            Data( int jointNumber );

            void onResponse( const RofiRespPtr & resp );
        };

        class RoFI::Joint::Data
        {
        public:
            friend class RoFI::Data;

        private:
            std::unordered_multimap< messages::JointCmd::Type, std::promise< RoFI::Data::RofiRespPtr > > respMap;
            std::mutex respMapMutex;

            std::vector< std::pair< std::function< bool( const messages::JointResp & ) >, std::function< void( Joint ) > > > respCallbacks;
            std::mutex respCallbacksMutex;

        public:
            Data( RoFI::Data & rofi, int jointNumber ) : rofi( rofi ), jointNumber( jointNumber ) {}

            RoFI::Data &rofi;
            int jointNumber;

            Joint getJoint();

            messages::RofiCmd getCmdMsg( messages::JointCmd::Type type ) const;
            std::future< RoFI::Data::RofiRespPtr > registerPromise( messages::JointCmd::Type type );

            void registerCallback( std::function< bool( const messages::JointResp & ) > && pred, std::function< void( Joint ) > && callback );

            void onResponse( const RoFI::Data::RofiRespPtr & resp );
        };
    }
}
