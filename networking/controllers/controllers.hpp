#pragma once

#include "rofi_interface.hpp"
#include "simulation.hpp"


namespace rofi::networking
{
void runRofiController( Simulation & simulation, RofiInterface & rofiInterface );
void runIntrospectionController();

} // namespace rofi::networking
