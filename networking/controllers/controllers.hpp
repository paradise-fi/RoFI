#pragma once

#include "rofi_interface.hpp"
#include "simulation.hpp"

#if __cplusplus >= 201911L
#include <jthread>
using jthread = std::jthread;
#else
#include "jthread.hpp"
using jthread = std20::jthread;
#endif


namespace rofi::networking
{
// Starts a new thread that runs the RoFI controller
[[nodiscard]] jthread runRofiController( Simulation & simulation, RofiInterface & rofiInterface );

// Starts a new thread that runs the introspection (master) controller
[[nodiscard]] jthread runIntrospectionController();

} // namespace rofi::networking
