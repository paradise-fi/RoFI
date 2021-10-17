#include "controllers.hpp"


namespace rofi::simplesim
{
Controller Controller::runRofiController( std::shared_ptr< Simulation > simulation,
                                          std::shared_ptr< Communication > communication )
{
    assert( simulation );
    assert( communication );

    return Controller( std::jthread( &Controller::rofiControllerThread,
                                     std::move( simulation ),
                                     std::move( communication ) ) );
}

void processRofiCommands( Simulation & simulation, Communication & communication )
{
    auto rofiCommands = communication.getRofiCommands();
    for ( auto & rofiCommand : rofiCommands ) {
        if ( auto response = simulation.processRofiCommand( *rofiCommand ) ) {
            communication.sendRofiResponses( std::array{ *response } );
        }
    }
}


void Controller::rofiControllerThread( std::stop_token stopToken,
                                       std::shared_ptr< Simulation > simulationPtr,
                                       std::shared_ptr< Communication > communicationPtr )
{
    assert( simulationPtr );
    assert( communicationPtr );
    auto & simulation = *simulationPtr;
    auto & communication = *communicationPtr;

    while ( !stopToken.stop_requested() ) {
        auto startTime = std::chrono::steady_clock::now();

        processRofiCommands( simulation, communication );
        auto eventResponses = simulation.simulateOneIteration();
        communication.sendRofiResponses( std::move( eventResponses ) );

        std::this_thread::sleep_until( startTime + Controller::updateDuration );
    }
}

} // namespace rofi::simplesim
