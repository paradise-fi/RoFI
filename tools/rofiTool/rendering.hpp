#pragma once
#include <configuration/rofiworld.hpp>

void renderRofiWorld( const rofi::configuration::RofiWorld & world,
                      const std::string & displayName = "Preview of Rofi world" );
void renderPoints( rofi::configuration::RofiWorld world,
                   const std::string & displayName = "Points of Rofi world",
                   bool showModules = false );
