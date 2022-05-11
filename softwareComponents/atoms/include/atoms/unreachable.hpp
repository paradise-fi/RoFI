#pragma once

#include <cassert>


#ifdef NDEBUG
    // `std::unreachable` from C++23
    #if defined( __GNUC__ ) // GCC, Clang, ICC
        #define ROFI_UNREACHABLE( description ) __builtin_unreachable()
    #elif defined( _MSC_VER ) // MSVC
        #define ROFI_UNREACHABLE( description ) __assume( false )
    #endif
#else
    #define ROFI_UNREACHABLE( description ) assert( false && description )
#endif
