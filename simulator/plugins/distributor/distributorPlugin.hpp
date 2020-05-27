#pragma once

#include <map>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "utils.hpp"

#include <distributorReq.pb.h>


namespace gazebo
{
class RofiDistributorPlugin : public WorldPlugin
{
    using RequestPtr = boost::shared_ptr< const rofi::messages::DistributorReq >;
    using RofiId = int;

public:
    RofiDistributorPlugin() = default;

    RofiDistributorPlugin( const RofiDistributorPlugin & ) = delete;
    RofiDistributorPlugin & operator=( const RofiDistributorPlugin & ) = delete;

    void Load( physics::WorldPtr world, sdf::ElementPtr sdf ) override;

    void loadRofis( sdf::ElementPtr pluginSdf );
    std::map< std::string, RofiId > loadRofisFromSdf( sdf::ElementPtr pluginSdf );

    void onRequest( const RequestPtr & req );
    void onAddEntity( std::string added );

private:
    physics::WorldPtr _world;

    transport::NodePtr _node;
    transport::PublisherPtr _pubOut;
    transport::SubscriberPtr _subOut;

    event::ConnectionPtr _onAddEntityConnection;

    std::map< RofiId, std::string > _rofiTopics;
    int _nextRofiId = 1;
    std::mutex _rofiTopicsMutex;
};

} // namespace gazebo
