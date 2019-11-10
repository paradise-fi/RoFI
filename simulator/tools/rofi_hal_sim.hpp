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
        namespace detail
        {
            constexpr double doublePrecision = 0.01;

            inline bool equal( double first, double second, double precision = doublePrecision )
            {
                return first <= second + precision && second <= first + precision;
            }

            class RoFIData
            {
            public:
                using RofiRespPtr = boost::shared_ptr< const messages::RofiResp >;

            private:
                gazebo::transport::NodePtr node;
                std::vector< std::unique_ptr< JointData > > joints;

            public:
                RoFIData( int jointNumber );

                Joint getJoint( int index );

                void onResponse( const RofiRespPtr & resp );

                gazebo::transport::PublisherPtr pub;
                gazebo::transport::SubscriberPtr sub;
            };

            class JointData
            {
            public:
                using Check = std::function< bool( const messages::JointResp & ) >;
                using Callback = std::function< void( Joint ) >;

                RoFIData & rofi;

            private:
                std::unordered_multimap< messages::JointCmd::Type, std::promise< RoFIData::RofiRespPtr > > respMap;
                std::mutex respMapMutex;

                std::pair< Check, Callback > respCallback;
                std::mutex respCallbackMutex;

                int jointNumber;

            public:
                JointData( RoFIData & rofi, int jointNumber ) : rofi( rofi ), jointNumber( jointNumber ) {}

                Joint getJoint();

                messages::RofiCmd getCmdMsg( messages::JointCmd::Type type );

                std::future< RoFIData::RofiRespPtr > registerPromise( messages::JointCmd::Type type );
                void registerCallback( Check && pred, Callback && callback );

                void onResponse( const RoFIData::RofiRespPtr & resp );
            };
        }
    }
}
