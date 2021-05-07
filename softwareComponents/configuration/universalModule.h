#pragma once

#include <array>
#include <iostream>
#include "rofibot.h"

namespace rofi {

enum UmParts { UmBodyA = 7, UmBodyB = 8, UmShoeA = 6, UmShoeB = 9 };

Module buildUniversalModule( double alpha, double beta, double gamma );

/**
 * Given a stream read a configuration from the old "Viki" format. Unlike the
 * original parser, supports only a single configuration per file.
 */
Rofibot readOldConfigurationFormat( std::istream& s );

} // namespace rofi