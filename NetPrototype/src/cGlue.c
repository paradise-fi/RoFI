#include <esp_wifi.h>

wifi_init_config_t getDefaultWifiCfg() {
    wifi_init_config_t t = WIFI_INIT_CONFIG_DEFAULT();
    return t;
}