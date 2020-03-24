#pragma once

#include <atomic>
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
            inline bool equal( double first, double second, double precision = 0.01 )
            {
                return first <= second + precision && second <= first + precision;
            }

            class RoFIData
            {
            public:
                using RofiRespPtr = boost::shared_ptr< const messages::RofiResp >;

            private:
                RoFI::Id id;
                gazebo::transport::NodePtr node;
                std::vector< std::unique_ptr< JointData > > joints;
                std::vector< std::unique_ptr< ConnectorData > > connectors;
                std::mutex descriptionMutex;
                std::atomic_bool hasDescription = false;

                mutable std::unordered_map< int, std::function< void() > > waitCallbacksMap;
                mutable std::mutex waitCallbacksMapMutex;
                mutable int waitId = 0;
            public:
                RoFIData( RoFI::Id id );

                RoFI::Id getId() const;
                Joint getJoint( int index );
                Connector getConnector( int index );

                void wait( int ms, std::function< void() > callback ) const;

                void onResponse( const RofiRespPtr & resp );

                gazebo::transport::PublisherPtr pub;
                gazebo::transport::SubscriberPtr sub;

            private:
                void getDescription();
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

                const int jointNumber;

            public:
                JointData( RoFIData & rofi, int jointNumber ) : rofi( rofi ), jointNumber( jointNumber ) {}

                Joint getJoint();

                messages::RofiCmd getCmdMsg( messages::JointCmd::Type type ) const;

                std::future< RoFIData::RofiRespPtr > registerPromise( messages::JointCmd::Type type );
                void registerCallback( Check && pred, Callback && callback );

                void onResponse( const RoFIData::RofiRespPtr & resp );
            };

            class ConnectorData
            {
            public:
                using Check = std::function< bool( const messages::ConnectorResp & ) >;
                using Callback = std::function< void( Connector, RoFIData::RofiRespPtr ) >;

                RoFIData & rofi;

            private:
                std::unordered_multimap< messages::ConnectorCmd::Type, std::promise< RoFIData::RofiRespPtr > > respMap;
                std::mutex respMapMutex;

                std::vector< std::pair< Check, Callback > > respCallbacks;
                std::mutex respCallbacksMutex;

                const int connectorNumber;

            public:
                ConnectorData( RoFIData & rofi, int connectorNumber ) : rofi( rofi ), connectorNumber( connectorNumber ) {}

                Connector getConnector();

                messages::RofiCmd getCmdMsg( messages::ConnectorCmd::Type type ) const;
                static Connector::Packet getPacket( RoFIData::RofiRespPtr );

                std::future< RoFIData::RofiRespPtr > registerPromise( messages::ConnectorCmd::Type type );
                void registerCallback( Check && pred, Callback && callback );

                void onResponse( const RoFIData::RofiRespPtr & resp );
            };
        }
    }
}
