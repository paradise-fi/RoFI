#pragma once

#include <array>
#include <iostream>
#include "rofibot.hpp"

namespace rofi {

enum UmParts { UmBodyA = 7, UmBodyB = 8, UmShoeA = 6, UmShoeB = 9 };

/**
 * Build an universal module with given angles of the respective joints.
 */
Module buildUniversalModule( Angle alpha, Angle beta, Angle gamma );

/**
 * Given a stream read a configuration from the old "Viki" format. Unlike the
 * original parser, supports only a single configuration per file.
 */
Rofibot readOldConfigurationFormat( std::istream& s );

} // namespace rofi

