#pragma once

#include "rofi_interface.hpp"
#include "simulation.hpp"

#include "jthread/jthread.hpp" // Change to std::jthread with C++20


namespace rofi::networking
{
// Starts a new thread that runs the RoFI controller
[[nodiscard]] std20::jthread runRofiController( Simulation & simulation, RofiInterface & rofiInterface );

// Starts a new thread that runs the introspection (master) controller
[[nodiscard]] std20::jthread runIntrospectionController();

} // namespace rofi::networking
