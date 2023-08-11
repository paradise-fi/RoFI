#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esp::delay {

inline void delayMs( int msCount ) {
	vTaskDelay( msCount / portTICK_PERIOD_MS );
}

inline void delayUs( int usCount ) {
	vTaskDelay( usCount / portTICK_PERIOD_MS / 1000 );
}

} // namespace esp::delay