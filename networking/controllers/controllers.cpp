#include "controllers.hpp"

#include <chrono>
#include <functional>
#include <thread>

#if __cplusplus >= 201911L
#include <stop_token>
using stop_token = std::stop_token;
#else
#include "stop_token.hpp"
using stop_token = std20::stop_token;
#endif


namespace rofi::networking
{
void rofiControllerThread( stop_token stopToken,
                           Simulation & simulation,
                           RofiInterface & rofiInterface )
{
    using namespace std::chrono_literals;
    using std::this_thread::sleep_for;

    for ( auto waitDuration = 0ms; !stopToken.stop_requested(); sleep_for( waitDuration ) )
    {
        auto lock = simulation.getWriteLock();

        waitDuration = simulation.getUpdateDuration();
        if ( !simulation.isRunning() )
        {
            continue;
        }

        auto rofiCommands = rofiInterface.getRofiCommands();
        std::vector< rofi::messages::RofiResp > processResponses;
        for ( auto & rofiCmd : rofiCommands )
        {
            if ( auto resp = simulation.processRofiCommand( *rofiCmd ) )
            {
                processResponses.push_back( std::move( *resp ) );
            }
        }

        auto eventResponses = simulation.moveRofisOneIteration();
        rofiInterface.sendRofiResponses( std::move( processResponses ) );
        rofiInterface.sendRofiResponses( std::move( eventResponses ) );
    }
}

jthread runRofiController( Simulation & simulation, RofiInterface & rofiInterface )
{
    return jthread( &rofiControllerThread, std::ref( simulation ), std::ref( rofiInterface ) );
}

void introspectionControllerThread()
{
}

jthread runIntrospectionController()
{
    return jthread( &introspectionControllerThread );
}

} // namespace rofi::networking
