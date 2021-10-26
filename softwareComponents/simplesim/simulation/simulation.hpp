#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <shared_mutex>

#include "command_handler.hpp"
#include "module_states.hpp"

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


    explicit Simulation( configuration::Rofibot && rofibotConfiguration )
            : _moduleStates( std::make_shared< ModuleStates >( std::move( rofibotConfiguration   ) ) )
            , _commandHandler( std::make_shared< CommandHandler >( this->_moduleStates ) )
    {
        assert( _moduleStates );
        assert( _commandHandler );
    }

    // Moves each rofi module based on the inner state
    // Returns the responses that happen inside RoFIs
    std::vector< RofiResp > simulateOneIteration()
    {
        assert( _moduleStates );

        _moduleStates->updateToNextIteration();
        auto responses = processRofiCommands();

        // TODO check for callbacks (position reached, connector events, waiting ended)

        return responses;
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

        auto commandCallbacks = _commandHandler->getCommandCallbacks();
        std::vector< RofiResp > responses;
        for ( auto & [ callback, rofiCmdPtr ] : commandCallbacks ) {
            assert( callback );
            assert( rofiCmdPtr );
            if ( auto resp = callback( *_moduleStates, *rofiCmdPtr ) ) {
                responses.push_back( std::move( *resp ) );
            }
        }
        return responses;
    }

private:
    std::shared_ptr< ModuleStates > _moduleStates;
    std::shared_ptr< CommandHandler > _commandHandler;
};

} // namespace rofi::simplesim
