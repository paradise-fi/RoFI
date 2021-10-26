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

        auto responses = simulation.simulateOneIteration();

        communication.sendRofiResponses( std::move( responses ) );

        std::this_thread::sleep_until( startTime + Controller::updateDuration );
    }
}

} // namespace rofi::simplesim
