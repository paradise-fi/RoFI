#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <vector>

#include <gazebo/transport/transport.hh>

#include <distributorReq.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class ModulesInfo
{
    template < typename T >
    using remove_cvref_t = std::remove_cv_t< std::remove_reference_t< T > >;

public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using SessionId = remove_cvref_t< decltype( rofi::messages::DistributorReq().sessionid() ) >;
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    class LockedModuleInfo
    {
    public:
        LockedModuleInfo( ModulesInfo & modulesInfo, RofiId rofiId, SessionId sessionId );

        LockedModuleInfo( LockedModuleInfo && other )
                : _modulesInfo( other._modulesInfo )
                , _rofiId( std::move( other._rofiId ) )
                , _sessionId( std::move( other._sessionId ) )
                , _topic( std::move( _topic ) )
                , _pub( std::move( _pub ) )
                , _sub( std::move( _sub ) )
        {
        }
        LockedModuleInfo & operator=( LockedModuleInfo && ) = delete;
        LockedModuleInfo( const LockedModuleInfo & ) = delete;
        LockedModuleInfo & operator=( const LockedModuleInfo & ) = delete;

        const std::string & topic() const
        {
            return _topic;
        }
        const SessionId & sessionId() const
        {
            return _sessionId;
        }
        gazebo::transport::Publisher & pub()
        {
            return *_pub;
        }
        gazebo::transport::Subscriber & sub()
        {
            return *_sub;
        }

    private:
        ModulesInfo & _modulesInfo;
        RofiId _rofiId = {};
        SessionId _sessionId = {};
        std::string _topic;
        gazebo::transport::PublisherPtr _pub;
        gazebo::transport::SubscriberPtr _sub;
    };

    ModulesInfo( gazebo::transport::NodePtr node ) : _node( node )
    {
        assert( _node );
    }
    ModulesInfo( const ModulesInfo & ) = delete;
    ModulesInfo & operator=( const ModulesInfo & ) = delete;

    // Returns true if the insertion was succesful
    // Returns false if the rofiId was already registered
    bool addNewRofi( RofiId rofiId );

    std::optional< RofiId > lockFreeRofi( SessionId sessionId );
    bool tryLockRofi( RofiId rofiId, SessionId sessionId );
    bool unlockRofi( RofiId rofiId, SessionId sessionId );

    std::optional< SessionId > getSessionId( RofiId rofiId ) const;
    std::optional< std::string > getTopic( RofiId rofiId ) const;
    bool isLocked( RofiId rofiId ) const;

    template < typename F >
    void forEachLockedModule( F function ) const
    {
        std::shared_lock< std::shared_mutex > lock( _modulesMutex );

        for ( auto & [ rofiId, moduleInfo ] : _lockedModules )
        {
            function( rofiId, moduleInfo.topic() );
        }
    }

    template < typename F >
    void forEachFreeModule( F function ) const
    {
        std::shared_lock< std::shared_mutex > lock( _modulesMutex );

        for ( auto rofiId : _freeModules )
        {
            function( rofiId );
        }
    }

    template < typename ResponsesContainer >
    void sendRofiResponses( ResponsesContainer && responses )
    {
        std::shared_lock< std::shared_mutex > lock1( _modulesMutex );

        for ( auto resp : responses )
        {
            auto it = _lockedModules.find( resp.rofiid() );
            it->second.pub().Publish( resp );
        }
    }

    auto getRofiCommands()
    {
        std::lock_guard< std::mutex > lock( _rofiCmdsMutex );

        auto result = std::move( _rofiCmds );
        _rofiCmds.clear();
        return result;
    }

private:
    void onRofiCmd( const RofiCmdPtr & msg )
    {
        std::lock_guard< std::mutex > lock( _rofiCmdsMutex );

        _rofiCmds.push_back( msg );
    }

    std::atomic_int topicCounter = 0;
    std::string getNewTopic()
    {
        return "~/rofi_uid_" + std::to_string( topicCounter++ );
    }


    LockedModuleInfo getNewLockedModuleInfo( SessionId sessionId );
    gazebo::transport::NodePtr _node;

    mutable std::shared_mutex _modulesMutex;
    std::set< RofiId > _freeModules;
    std::map< RofiId, LockedModuleInfo > _lockedModules;

    std::mutex _rofiCmdsMutex;
    std::vector< RofiCmdPtr > _rofiCmds;
};

} // namespace rofi::simplesim
