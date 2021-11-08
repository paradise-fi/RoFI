#include <system/assert.hpp>
#include <system/dbg.hpp>

void backtrace();

extern "C" void __assert_func( const char *file,
			   int line, const char *function, const char *assertion )
{
    Dbg::error( "Assert failed: %s\n  in %s\n     (%s:%d)", assertion, function, file, line );
    backtrace();
    Dbg::error( "Halt" );
    while ( true );
}
