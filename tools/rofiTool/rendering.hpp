#pragma once
#include <configuration/rofiworld.hpp>

void renderConfiguration( rofi::configuration::RofiWorld world,
                          const std::string & configName = "unknown" );
void renderPoints( rofi::configuration::RofiWorld world,
                   const std::string & configName = "unknown",
                   bool showModules = false );
