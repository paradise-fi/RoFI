#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "atoms/guarded.hpp"
#include "module_states.hpp"

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

    std::optional< RofiResp > onRofiCmd( const RofiCmdPtr & rofiCmdPtr )
    {
        assert( rofiCmdPtr );
        assert( _moduleStates );

        auto callbacks = onRofiCmdCallbacks( *rofiCmdPtr );

        if ( callbacks.delayed ) {
            _rofiCmdCallbacks->emplace_back( std::move( callbacks.delayed ), rofiCmdPtr );
        }

        if ( callbacks.immediate ) {
            return callbacks.immediate( *_moduleStates, *rofiCmdPtr );
        }
        return std::nullopt;
    }

    auto getModuleIds()
    {
        assert( _moduleStates );
        return _moduleStates->getModuleIds();
    }

    std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > getCommandCallbacks()
    {
        return _rofiCmdCallbacks.visit( []( auto & vec ) {
            auto result = std::move( vec );
            vec.clear();
            return result;
        } );
    }


private:
    static CommandCallbacks onJointCmdCallbacks( JointCmd::Type cmd_type );
    static CommandCallbacks onConnectorCmdCallbacks( ConnectorCmd::Type cmd_type );
    static CommandCallbacks onRofiCmdCallbacks( const RofiCmd & cmd );

    std::shared_ptr< const ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
