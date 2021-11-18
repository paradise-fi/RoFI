#pragma once

#include <thread>

#include "rofi_interface.hpp"
#include "simulation.hpp"


namespace rofi::networking
{
// Starts a new thread that runs the RoFI controller
[[nodiscard]] std::jthread runRofiController( Simulation & simulation, RofiInterface & rofiInterface );

// Starts a new thread that runs the introspection (master) controller
[[nodiscard]] std::jthread runIntrospectionController();

} // namespace rofi::networking
