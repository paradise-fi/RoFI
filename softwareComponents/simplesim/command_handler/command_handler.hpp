#pragma once

#include <functional>
#include <optional>
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
    using CommandCallback = std::function< std::optional< RofiResp >() >;

    class CommandCallbacks
    {
    public:
        CommandCallback immediate;
        CommandCallback delayed;
    };

    CommandHandler( std::shared_ptr< ModuleStates > moduleStates )
            : _moduleStates( std::move( moduleStates ) )
    {
        assert( _moduleStates );
    }

    std::vector< RofiResp > processRofiCommands()
    {
        std::vector< RofiResp > responses;
        for ( auto & callback : getCommandCallbacks() ) {
            assert( callback );
            if ( auto resp = callback() ) {
                responses.push_back( std::move( *resp ) );
            }
        }
        return responses;
    }

    std::optional< RofiResp > onRofiCmd( const RofiCmdPtr & msg )
    {
        assert( msg );
        auto callbacks = onRofiCmdCallbacks( *msg );

        if ( callbacks.delayed ) {
            _rofiCmdCallbacks->push_back( std::move( callbacks.delayed ) );
        }

        if ( callbacks.immediate ) {
            return callbacks.immediate();
        }
        return std::nullopt;
    }

    auto getModuleIds()
    {
        assert( _moduleStates );
        return _moduleStates->getModuleIds();
    }

private:
    CommandCallbacks onJointCmdCallbacks( ModuleId moduleId, const JointCmd & cmd );
    CommandCallbacks onConnectorCmdCallbacks( ModuleId moduleId, const ConnectorCmd & cmd );
    CommandCallbacks onRofiCmdCallbacks( const RofiCmd & cmd );

    std::vector< CommandCallback > getCommandCallbacks()
    {
        return _rofiCmdCallbacks.visit( []( auto & vec ) {
            auto result = std::move( vec );
            vec.clear();
            return result;
        } );
    }


    std::shared_ptr< ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< CommandCallback > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
