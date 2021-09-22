#include <FreeRTOS.h>
#include <hal.h>

int main() {
    halInit();
    osalInit();

    vTaskStartScheduler();
    __builtin_unreachable();
}