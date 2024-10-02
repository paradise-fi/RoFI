#include <stm32g0xx_hal.h>
#include <system/dbg.hpp>

int main() {
    HAL_Init();

    Dbg::info("Test started");

    Dbg::info("Test finished");
    while( true );
}
