// #include "esp_wifi.h"

// #ifndef _ESP_NETIF_H_

// #include "esp_log.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/event_groups.h"
// #include "nvs_flash.h"
// #include <string.h>

// #include "rbwifi.h"

// #define TAG "RbWifi"

// namespace rb {

// class WiFiInitializer {
//     friend class WiFi;

// public:
//     WiFiInitializer() {
//         tcpip_adapter_init();

//         ESP_ERROR_CHECK(esp_event_loop_init(&WiFi::eventHandler_tcpip, NULL));

//         wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//         cfg.nvs_enable = 0;
//         cfg.nano_enable = 1;
//         ESP_ERROR_CHECK(esp_wifi_init(&cfg));

//         ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
//     }

//     ~WiFiInitializer() {
//     }
// };

// std::atomic<uint32_t> WiFi::m_ip;

// void WiFi::init() {
//     static WiFiInitializer init;
// }

// void WiFi::connect(const char* ssid, const char* pass) {
//     init();

//     esp_wifi_stop();

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

//     wifi_config_t cfg = {};
//     snprintf((char*)cfg.sta.ssid, 32, "%s", ssid);
//     snprintf((char*)cfg.sta.password, 64, "%s", pass);
//     esp_wifi_set_config(WIFI_IF_STA, &cfg);

//     ESP_ERROR_CHECK(esp_wifi_start());
// }

// void WiFi::startAp(const char* ssid, const char* pass, uint8_t channel) {
//     init();

//     esp_wifi_stop();

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

//     wifi_config_t cfg = {};

//     if (strlen(pass) >= 8) {
//         snprintf((char*)cfg.ap.password, 64, "%s", pass);
//         cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
//     } else {
//         ESP_LOGE(TAG, "The WiFi password is too short, 8 characters required, leaving the WiFI open!");
//         cfg.ap.authmode = WIFI_AUTH_OPEN;
//     }
//     snprintf((char*)cfg.ap.ssid, 32, "%s", ssid);
//     cfg.ap.channel = channel;
//     cfg.ap.beacon_interval = 400;
//     cfg.ap.max_connection = 4;
//     ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));

//     ESP_ERROR_CHECK(esp_wifi_start());

//     esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
// }

// esp_err_t WiFi::eventHandler_tcpip(void* ctx, system_event_t* event) {
//     switch (event->event_id) {
//     case SYSTEM_EVENT_STA_START:
//         ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
//         tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA);
//         ESP_ERROR_CHECK(esp_wifi_connect());
//         break;
//     case SYSTEM_EVENT_STA_STOP:
//         ESP_LOGI(TAG, "SYSTEM_EVENT_STA_STOP");
//         tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
//         break;
//     case SYSTEM_EVENT_STA_GOT_IP: {
//         ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
//         ESP_LOGI(TAG, "Got IP: %s\n", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
//         m_ip.store(event->event_info.got_ip.ip_info.ip.addr);
//         break;
//     }
//     case SYSTEM_EVENT_STA_DISCONNECTED:
//         m_ip.store(0);
//         ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
//         esp_wifi_connect();
//         break;
//     case SYSTEM_EVENT_AP_START: {
//         ESP_LOGI(TAG, "SYSTEM_EVENT_AP_START");
//         tcpip_adapter_ip_info_t info;
//         IP4_ADDR(&info.ip, 192, 168, 0, 1);
//         IP4_ADDR(&info.gw, 192, 168, 0, 1);
//         IP4_ADDR(&info.netmask, 255, 255, 255, 0);
//         tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
//         ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));

//         dhcps_lease_t lease;
//         lease.enable = true;
//         IP4_ADDR(&lease.start_ip, 192, 168, 0, 10);
//         IP4_ADDR(&lease.end_ip, 192, 168, 0, 250);

//         tcpip_adapter_dhcps_option(
//             TCPIP_ADAPTER_OP_SET,
//             TCPIP_ADAPTER_REQUESTED_IP_ADDRESS,
//             (void*)&lease, sizeof(dhcps_lease_t));

//         ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

//         m_ip.store(info.ip.addr);
//         break;
//     }
//     case SYSTEM_EVENT_AP_STOP:
//         ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STOP");
//         tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
//         m_ip.store(0);
//         break;
//     case SYSTEM_EVENT_AP_STACONNECTED:
//         ESP_LOGI(TAG, "SYSTEM_EVENT_AP_STACONNECTED");
//         break;
//     default:
//         ESP_LOGI(TAG, "Unhandled event %d", (int)event->event_id);
//         break;
//     }
//     return ESP_OK;
// }

// }; // namespace rb

// #endif
