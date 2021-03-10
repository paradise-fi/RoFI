#include <cassert>
#include <system/dbg.hpp>

extern "C" void __assert_func( const char *file,
			   int line, const char *function, const char *assertion )
{
    Dbg::error( "Assert failed: %s\n  in  %s (%s:%d)", assertion, function, file, line );
    Dbg::info( "Halt" );
    while ( true );
}
