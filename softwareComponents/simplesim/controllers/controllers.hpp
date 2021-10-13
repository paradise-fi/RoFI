#pragma once

#include "rofi_interface.hpp"
#include "simulation.hpp"

#include "atoms/jthread.hpp"


namespace rofi::networking
{
// Starts a new thread that runs the RoFI controller
[[nodiscard]] atoms::jthread runRofiController( Simulation & simulation, RofiInterface & rofiInterface );

// Starts a new thread that runs the introspection (master) controller
[[nodiscard]] atoms::jthread runIntrospectionController();

} // namespace rofi::networking
