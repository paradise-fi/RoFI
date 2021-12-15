#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <optional>
#include <shared_mutex>
#include <utility>

#include "atoms/guarded.hpp"
#include "atoms/patterns.hpp"
#include "command_handler.hpp"
#include "module_states.hpp"
#include "wait_handler.hpp"

#include <connectorCmd.pb.h>
#include <jointCmd.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
/** Class that handles and encapsulates the simulation
 */
class Simulation
{
public:
    using RofiResp = rofi::messages::RofiResp;


    explicit Simulation(
            std::shared_ptr< const rofi::configuration::Rofibot > rofibotConfiguration )
            : _moduleStates( std::make_shared< ModuleStates >( std::move( rofibotConfiguration ) ) )
            , _commandHandler( std::make_shared< CommandHandler >( this->_moduleStates ) )
    {
        assert( _moduleStates );
        assert( _commandHandler );
    }

    // Moves each rofi module based on the inner state
    // Returns the responses that happen inside RoFIs
    std::pair< std::vector< RofiResp >, std::shared_ptr< const rofi::configuration::Rofibot > >
            simulateOneIteration( std::chrono::milliseconds duration )
    {
        assert( _moduleStates );

        auto responses = processRofiCommands();
        auto new_configuration =
                _moduleStates->updateToNextIteration( duration, [ &responses ]( RofiResp resp ) {
                    responses.push_back( std::move( resp ) );
                } );
        assert( new_configuration );

        std::ranges::transform( _commandHandler->waitHandler()->advanceTime( duration ),
                                std::back_inserter( responses ),
                                WaitHandler::getWaitResp );

        return std::make_pair( std::move( responses ), std::move( new_configuration ) );
    }

    std::shared_ptr< CommandHandler > commandHandler()
    {
        assert( _commandHandler );
        return _commandHandler;
    }

private:
    std::vector< RofiResp > processRofiCommands()
    {
        assert( _commandHandler );
        assert( _moduleStates );

        auto commandCallbacks = _commandHandler->extractCommandCallbacks();
        std::vector< RofiResp > responses;
        for ( auto & [ callback, rofiCmdPtr ] : commandCallbacks ) {
            using SendPacketEvent = CommandHandler::SendPacketEvent;
            using DisconnectEvent = CommandHandler::DisconnectEvent;

            assert( callback );
            assert( rofiCmdPtr );
            std::visit( overload{ []( std::nullopt_t /* nothing */ ) {},
                                  [ &responses ]( SendPacketEvent && packet_event ) {
                                      auto resp = packet_event.receiver.getRofiResp(
                                              rofi::messages::ConnectorCmd::PACKET );
                                      *resp.mutable_connectorresp()->mutable_packet() = std::move(
                                              packet_event.packet );
                                      // TODO use packet filter
                                      responses.push_back( std::move( resp ) );
                                  },
                                  [ &responses ]( const DisconnectEvent & disconnect_event ) {
                                      responses.push_back( disconnect_event.first.getRofiResp(
                                              rofi::messages::ConnectorCmd::DISCONNECT ) );
                                      responses.push_back( disconnect_event.second.getRofiResp(
                                              rofi::messages::ConnectorCmd::DISCONNECT ) );
                                  } },
                        callback( *_moduleStates, *rofiCmdPtr ) );
        }
        return responses;
    }

private:
    std::shared_ptr< ModuleStates > _moduleStates;
    std::shared_ptr< CommandHandler > _commandHandler;
};

} // namespace rofi::simplesim
