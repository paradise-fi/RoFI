#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>

#include <atoms/guarded.hpp>
#include <gazebo/transport/transport.hh>

#include "distributor.hpp"

#include <distributorReq.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication;

class LockedModuleCommunication
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    LockedModuleCommunication( ModulesCommunication & modulesCommunication, RofiId rofiId );

    std::string topic( const gazebo::transport::Node & node ) const
    {
        return "/gazebo/" + node.GetTopicNamespace() + "/" + _topicName;
    }
    gazebo::transport::Publisher & pub()
    {
        assert( _pub );
        return *_pub;
    }
    gazebo::transport::Subscriber & sub()
    {
        assert( _sub );
        return *_sub;
    }

private:
    void onRofiCmd( const RofiCmdPtr & msg );

private:
    ModulesCommunication & _modulesCommunication;
    RofiId _rofiId = {};
    std::string _topicName;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};


class ModulesCommunication
{
public:
    using RofiId = LockedModuleCommunication::RofiId;
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    ModulesCommunication( gazebo::transport::NodePtr node, std::set< RofiId > rofiIds )
            : _node( node )
            , _freeModules( std::move( rofiIds ) )
            , _distributor( *this->_node, *this )

    {
        assert( _node );
    }
    ModulesCommunication( gazebo::transport::NodePtr node ) : ModulesCommunication( node, {} ) {}

    ModulesCommunication( const ModulesCommunication & ) = delete;
    ModulesCommunication & operator=( const ModulesCommunication & ) = delete;

    // Returns true if the insertion was succesful
    // Returns false if the rofiId was already registered
    bool addNewRofi( RofiId rofiId );

    std::optional< RofiId > lockFreeRofi();
    bool tryLockRofi( RofiId rofiId );
    bool unlockRofi( RofiId rofiId );

    std::optional< std::string > getTopic( RofiId rofiId ) const;
    bool isLocked( RofiId rofiId ) const;

    template < typename F >
    void forEachLockedModule( F && function ) const
    {
        std::shared_lock< std::shared_mutex > lock( _modulesMutex );

        for ( auto & [ rofiId, moduleComm ] : _lockedModules ) {
            function( rofiId, moduleComm.topic( *_node ) );
        }
    }

    template < typename F >
    void forEachFreeModule( F && function ) const
    {
        std::shared_lock< std::shared_mutex > lock( _modulesMutex );

        for ( auto rofiId : _freeModules ) {
            function( rofiId );
        }
    }

    template < typename ResponsesContainer >
    void sendRofiResponses( ResponsesContainer && responses )
    {
        std::shared_lock< std::shared_mutex > lock1( _modulesMutex );

        for ( auto resp : responses ) {
            auto it = _lockedModules.find( resp.rofiid() );
            it->second.pub().Publish( std::move( resp ) );
        }
    }

    auto getRofiCommands()
    {
        auto result = std::vector< RofiCmdPtr >();
        _rofiCmds->swap( result );
        return result;
    }

private:
    friend class LockedModuleCommunication;

    void onRofiCmd( const RofiCmdPtr & msg )
    {
        _rofiCmds->push_back( msg );
    }

    std::atomic_int topicNameCounter = 0;
    std::string getNewTopicName()
    {
        return "rofi_uid_" + std::to_string( topicNameCounter++ );
    }


    LockedModuleCommunication getNewLockedModule();

private:
    gazebo::transport::NodePtr _node;

    mutable std::shared_mutex _modulesMutex;
    std::set< RofiId > _freeModules;
    std::map< RofiId, LockedModuleCommunication > _lockedModules;

    atoms::Guarded< std::vector< RofiCmdPtr > > _rofiCmds;

    Distributor _distributor;
};

} // namespace rofi::simplesim
