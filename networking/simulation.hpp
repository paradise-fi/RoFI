#pragma once

namespace rofi::networking
{
using RofiId = int;

/** Class that handles and encapsulates the simulation
 */
class Simulation
{
public:
    // Adds a new RoFI module with a new unique Id
    void addModule();

    // Adds a new RoFI module with given Id
    // throws if given Id already exists
    void addModule( RofiId rofiId );

    // Runs one iteration of simulation
    // TODO what to do on collision
    void runIteration();
};

} // namespace rofi::networking
