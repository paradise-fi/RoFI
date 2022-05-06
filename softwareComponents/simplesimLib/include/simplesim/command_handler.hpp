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
#include "delayed_data_handler.hpp"
#include "module_states.hpp"
#include "packet_filter.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class CommandHandler {
public:
    struct DisconnectEvent {
        Connector first;
        Connector second;
    };

    struct WaitData {
        ModuleId moduleId;
        int waitId;

        rofi::messages::RofiResp getRofiResp() const
        {
            rofi::messages::RofiResp rofiResp;
            rofiResp.set_rofiid( this->moduleId );
            rofiResp.set_resptype( rofi::messages::RofiCmd::WAIT_CMD );
            rofiResp.set_waitid( this->waitId );
            return rofiResp;
        }
    };

    enum class DelayedDataType
    {
        None = 0,
        Wait,
        SendPacket
    };

    using RofiCmd = rofi::messages::RofiCmd;
    using RofiCmdPtr = boost::shared_ptr< const RofiCmd >;

    using ImmediateCmdCallback = std::function<
            std::optional< rofi::messages::RofiResp >( const ModuleStates &, const RofiCmd & ) >;
    using DelayedEvent = std::variant< std::nullopt_t, DisconnectEvent >;
    using DelayedCmdCallback = std::function< DelayedEvent( ModuleStates &, const RofiCmd & ) >;

    struct CommandCallbacks {
        ImmediateCmdCallback immediate = {};
        DelayedCmdCallback delayed = {};
        DelayedDataType delayedDataType = DelayedDataType::None;
    };


    CommandHandler( std::shared_ptr< ModuleStates > moduleStates,
                    PacketFilter::FilterFunction packetFilter )
            : _moduleStates( std::move( moduleStates ) ), _packetFilter( std::move( packetFilter ) )
    {
        assert( _moduleStates );
    }

    std::optional< rofi::messages::RofiResp > onRofiCmd( RofiCmdPtr rofiCmdPtr );

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

    void advanceTime( std::chrono::milliseconds duration,
                      std::invocable< rofi::messages::RofiResp > auto callback )
    {
        auto waitResponses = _waitHandler->advanceTime( duration );
        auto packetResponses = _packetFilter->advanceTime( duration );

        for ( const auto & waitData : waitResponses ) {
            callback( waitData.getRofiResp() );
        }

        for ( const auto & packetData : packetResponses ) {
            callback( packetData.getRofiResp() );
        }
    }

private:
    void onDelayedData( DelayedDataType delayedDataType, const RofiCmd & cmd );

    std::shared_ptr< const ModuleStates > _moduleStates;
    atoms::Guarded< DelayedDataHandler< WaitData > > _waitHandler;
    atoms::Guarded< PacketFilter > _packetFilter;

    atoms::Guarded< std::vector< std::pair< DelayedCmdCallback, RofiCmdPtr > > > _rofiCmdCallbacks;
};

} // namespace rofi::simplesim
