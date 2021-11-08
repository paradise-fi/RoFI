#pragma once

#include <cassert>

#undef assert

#ifdef NDEBUG
    #define assert(__e) ((void)0)
    #define impossible(msg) __builtin_trap()
#else
    #define assert(__e) (                                          \
        (__e)                                                      \
            ? (void)0                                              \
            : __assert_func( __FILE__, __LINE__,              \
                             __PRETTY_FUNCTION__, #__e ) )

    #define impossible(msg) do {                                   \
        __assert_func( __BASE_FILE__, __LINE__,                    \
                       __PRETTY_FUNCTION__, msg );                 \
        __builtin_unreachable();                                  \
    } while ( false )
#endif

extern "C" void __assert_func( const char *file,
			   int line, const char *function, const char *assertion );
