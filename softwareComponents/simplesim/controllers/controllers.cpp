#include "controllers.hpp"


namespace rofi::simplesim
{
Controller Controller::runRofiController( std::shared_ptr< Simulation > simulation,
                                          std::shared_ptr< RofiInterface > rofiInterface )
{
    assert( simulation );
    assert( rofiInterface );

    return Controller( std::jthread( &Controller::rofiControllerThread,
                                     std::move( simulation ),
                                     std::move( rofiInterface ) ) );
}

void processRofiCommands( Simulation & simulation, RofiInterface & rofiInterface )
{
    auto rofiCommands = rofiInterface.getRofiCommands();
    for ( auto & rofiCommand : rofiCommands ) {
        if ( auto response = simulation.processRofiCommand( *rofiCommand ) ) {
            rofiInterface.sendRofiResponses( std::array{ *response } );
        }
    }
}


void Controller::rofiControllerThread( std::stop_token stopToken,
                                       std::shared_ptr< Simulation > simulationPtr,
                                       std::shared_ptr< RofiInterface > rofiInterfacePtr )
{
    assert( simulationPtr );
    assert( rofiInterfacePtr );
    auto & simulation = *simulationPtr;
    auto & rofiInterface = *rofiInterfacePtr;

    while ( !stopToken.stop_requested() ) {
        auto startTime = std::chrono::steady_clock::now();

        processRofiCommands( simulation, rofiInterface );
        auto eventResponses = simulation.simulateOneIteration();
        rofiInterface.sendRofiResponses( std::move( eventResponses ) );

        std::this_thread::sleep_until( startTime + Controller::updateDuration );
    }
}

} // namespace rofi::simplesim
