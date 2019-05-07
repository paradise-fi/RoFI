#include <stm32g0xx_hal.h>

#include <cassert>
#include <system/dbg.hpp>
#include <system/clock.hpp>

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();

    Dbg::info( "Started: %d", SystemCoreClock );
    int counter = 0;
    while ( true ) {
        Dbg::info( "Tick %d", counter );
        counter++;
        HAL_Delay( 500 );
        if ( counter == 2 )
            assert( false && "Cool message" );
    }
}

