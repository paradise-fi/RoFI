#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "atoms/guarded.hpp"
#include "module_states.hpp"
#include "wait_handler.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class CommandHandler
{
    using RofiCmd = rofi::messages::RofiCmd;
    using JointCmd = rofi::messages::JointCmd;
    using ConnectorCmd = rofi::messages::ConnectorCmd;

public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;
    using RofiResp = rofi::messages::RofiResp;
    using ImmediateCmdCallback =
            std::function< std::optional< RofiResp >( const ModuleStates &, const RofiCmd & ) >;
    using DelayedCmdCallback =
            std::function< std::optional< RofiResp >( ModuleStates &, const RofiCmd & ) >;

    class CommandCallbacks
    {
    public:
        ImmediateCmdCallback immediate;
        DelayedCmdCallback delayed;
    };


    CommandHandler( std::shared_ptr< ModuleStates > moduleStates )
            : _moduleStates( std::move( moduleStates ) )
    {
        assert( _moduleStates );
    }

    std::optional< RofiResp > onRofiCmd( const RofiCmdPtr & rofiCmdPtr );

    auto getModuleIds()
    {
        assert( _moduleStates );
        return _moduleStates->getModuleIds();
    }

    std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > extractCommandCallbacks()
    {
        return _rofiCmdCallbacks.visit( []( auto & vec ) {
            auto result = std::move( vec );
            vec.clear();
            return result;
        } );
    }

    atoms::Guarded< WaitHandler > & waitHandler()
    {
        return _waitHandler;
    }
    const atoms::Guarded< WaitHandler > & waitHandler() const
    {
        return _waitHandler;
    }

private:
    void onWaitCmd( const RofiCmd & rofiCmd );
    CommandCallbacks onRofiCmdCallbacks( const RofiCmd & cmd );

    atoms::Guarded< WaitHandler > _waitHandler;
    std::shared_ptr< const ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
