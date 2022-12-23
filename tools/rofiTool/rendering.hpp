#pragma once

#include <span>

#include <configuration/rofiworld.hpp>


void renderRofiWorld( const rofi::configuration::RofiWorld & world,
                      const std::string & displayName = "Preview of Rofi world" );
void renderRofiWorldSequence( std::span< const rofi::configuration::RofiWorld > worlds,
                              const std::string & displayName = "Preview of Rofi world sequence" );
void renderPoints( rofi::configuration::RofiWorld world,
                   const std::string & displayName = "Points of Rofi world",
                   bool showModules = false );
