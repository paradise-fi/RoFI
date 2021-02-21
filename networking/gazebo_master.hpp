#pragma once

#include <memory>
#include <string>

#include "gazebo/Master.hh"

namespace rofi::networking
{
// Starts Gazebo master for communication in a separate thread
[[nodiscard]] std::unique_ptr< gazebo::Master > startGazeboMaster();

} // namespace rofi::networking
