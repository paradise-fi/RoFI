#pragma once

#include <atomic>
#include <chrono>
#include <optional>
#include <shared_mutex>

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


    explicit Simulation( rofi::configuration::Rofibot && rofibotConfiguration )
            : _moduleStates( std::move( rofibotConfiguration ) )
    {}

    // Moves each rofi module based on the inner state
    // Returns the responses that happen inside RoFIs
    std::vector< RofiResp > simulateOneIteration();

    std::optional< RofiResp > processRofiCommand( const rofi::messages::RofiCmd & cmd );

    std::set< ModuleId > getModuleIds() const
    {
        return _moduleStates.getModuleIds();
    }

private:
    std::optional< RofiResp > processJointCommand( ModuleId moduleId,
                                                   const rofi::messages::JointCmd & cmd );
    std::optional< RofiResp > processConnectorCommand( ModuleId moduleId,
                                                       const rofi::messages::ConnectorCmd & cmd );

    static RofiResp getJointResp( ModuleId moduleId,
                                  int joint,
                                  rofi::messages::JointCmd::Type type,
                                  float value = 0.f );
    static RofiResp getConnectorResp( ModuleId moduleId,
                                      int connector,
                                      rofi::messages::ConnectorCmd::Type type );


    ModuleStates _moduleStates;
};

} // namespace rofi::simplesim
