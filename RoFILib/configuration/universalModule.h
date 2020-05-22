#pragma once

#include <array>
#include "rofibot.h"

namespace rofi {

enum UmParts { UmBodyA = 7, UmBodyB = 8, UmShoeA = 6, UmShoeB = 9 };

Module buildUniversalModule( double alpha, double beta, double gamma );


} // namespace rofi