#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "atoms/guarded.hpp"
#include "module_states.hpp"
#include "delayed_data_handler.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class CommandHandler
{
public:
    struct DisconnectEvent
    {
        Connector first;
        Connector second;
    };

    struct SendPacketEvent
    {
        Connector sender;
        Connector receiver;
        rofi::messages::Packet packet;
    };

    struct WaitEvent
    {
        ModuleId moduleId;
        int waitId;
        int waitMs;

        rofi::messages::RofiResp getRofiResp() const
        {
            rofi::messages::RofiResp rofiResp;
            rofiResp.set_rofiid( this->moduleId );
            rofiResp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
            rofiResp.set_waitid( this->waitId );
            return rofiResp;
        }
    };

    using RofiCmd = rofi::messages::RofiCmd;
    using RofiCmdPtr = boost::shared_ptr< const RofiCmd >;

    using ImmediateCmdCallback = std::function<
            std::optional< rofi::messages::RofiResp >( const ModuleStates &, const RofiCmd & ) >;
    using DelayedEvent = std::variant< std::nullopt_t, DisconnectEvent, SendPacketEvent >;
    using DelayedCmdCallback = std::function< DelayedEvent( ModuleStates &, const RofiCmd & ) >;
    using DelayedData = std::optional< WaitEvent >;

    struct CommandCallbacks
    {
        ImmediateCmdCallback immediate = {};
        DelayedCmdCallback delayed = {};
        DelayedData delayedData = std::nullopt;
    };


    CommandHandler( std::shared_ptr< ModuleStates > moduleStates )
            : _moduleStates( std::move( moduleStates ) )
    {
        assert( _moduleStates );
    }

    std::optional< rofi::messages::RofiResp > onRofiCmd( const RofiCmdPtr & rofiCmdPtr );

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

    void advanceTime( std::chrono::milliseconds duration, std::invocable< rofi::messages::RofiResp > auto callback )
    {
        auto waitResponses = _waitHandler->advanceTime( duration );

        for ( const WaitEvent & waitEvent : waitResponses ) {
            callback( waitEvent.getRofiResp() );
        }
    }

private:
    CommandCallbacks onRofiCmdCallbacks( const RofiCmd & cmd );

    atoms::Guarded< DelayedDataHandler< WaitEvent > > _waitHandler;
    std::shared_ptr< const ModuleStates > _moduleStates;

    atoms::Guarded< std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
