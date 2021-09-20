#pragma once

#include <chrono>
#include <optional>
#include <shared_mutex>

#include "database.hpp"

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
    static constexpr auto defaultUpdateDuration = std::chrono::milliseconds( 100 );

public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using WriteLock = std::unique_lock< std::shared_mutex >;
    using ReadLock = std::shared_lock< std::shared_mutex >;
    using OptionalRofiResp = std::optional< rofi::messages::RofiResp >;

    // Adds a new RoFI module with a new unique Id
    RofiId addModule();

    // Adds a new RoFI module with given Id
    // throws if given Id already exists
    void addModule( RofiId rofiId, int joints, int connectors );

    // Moves each rofi module based on the internal state
    // Returns the responses that happen inside RoFIs
    std::vector< rofi::messages::RofiResp > moveRofisOneIteration();

    bool isRunning() const
    {
        return _isRunning;
    }

    // Returns the real time between updates
    std::chrono::milliseconds getUpdateDuration() const
    {
        // TODO make configurable
        return defaultUpdateDuration;
    }

    OptionalRofiResp processRofiCommand( const rofi::messages::RofiCmd & cmd );
    OptionalRofiResp processJointCommand( RofiId rofiId, const rofi::messages::JointCmd & cmd );
    OptionalRofiResp processConnectorCommand( RofiId rofiId,
                                              const rofi::messages::ConnectorCmd & cmd );

    ReadLock getReadLock() const
    {
        return ReadLock( _mutex );
    }

    WriteLock getWriteLock()
    {
        return WriteLock( _mutex );
    }

private:
    static rofi::messages::RofiResp getJointResp( RofiId rofiId,
                                                  int joint,
                                                  rofi::messages::JointCmd::Type type,
                                                  float value = 0.f );
    static rofi::messages::RofiResp getConnectorResp( RofiId rofiId,
                                                      int connector,
                                                      rofi::messages::ConnectorCmd::Type type );


    bool _isRunning = true;

    mutable std::shared_mutex _mutex;
    Database _database;
};

} // namespace rofi::simplesim
