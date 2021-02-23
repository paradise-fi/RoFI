#pragma once

#include <chrono>
#include <shared_mutex>
#include <vector>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::networking
{
using RofiId = int;

/** Class that handles and encapsulates the simulation
 */
class Simulation
{
public:
    using WriteLock = std::unique_lock< std::shared_mutex >;
    using ReadLock = std::shared_lock< std::shared_mutex >;

    // Adds a new RoFI module with a new unique Id
    RofiId addModule();

    // Adds a new RoFI module with given Id
    // throws if given Id already exists
    void addModule( RofiId rofiId );

    // Moves each rofi module based on the internal state
    // Returns the responses that happen inside RoFIs
    std::vector< rofi::messages::RofiResp > moveRofisOneIteration();

    bool isRunning() const
    {
        return {};
    }

    // Returns the real time between updates
    std::chrono::milliseconds getUpdateDuration() const
    {
        return {};
    }

    void processRofiCommand( const rofi::messages::RofiCmd & cmd );

    ReadLock getReadLock() const
    {
        return ReadLock( _mutex );
    }

    WriteLock getWriteLock()
    {
        return WriteLock( _mutex );
    }

private:
    mutable std::shared_mutex _mutex;
};

} // namespace rofi::networking
