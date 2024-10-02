#pragma once

#ifndef UM_BOARD_VER_MAJ
    #define UM_BOARD_VER_MAJ 0
    #define UM_BOARD_VER_MIN 2
#endif

#if UM_BOARD_VER_MAJ == 0 && UM_BOARD_VER_MIN == 2
    #include "bsp/v0.2.hpp"
#else
    #error "Unsupported Universal Module board version"
#endif
