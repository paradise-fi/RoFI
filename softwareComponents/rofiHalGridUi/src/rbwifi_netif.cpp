#include "esp_wifi.h"

#ifdef _ESP_NETIF_H_

#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include <string.h>

#include "rbwifi.h"

#define TAG "RbWifi"

static esp_netif_t* gNetIf = nullptr;

namespace rb {

class WiFiInitializer {
    friend class WiFi;

public:
    WiFiInitializer() {
        ESP_ERROR_CHECK(nvs_flash_init());
        ESP_ERROR_CHECK(esp_netif_init());

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &WiFi::eventHandler_netif,
            NULL,
            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &WiFi::eventHandler_netif,
            NULL,
            &instance_got_ip));

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        cfg.nvs_enable = 0;
        cfg.nano_enable = 1;
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    }

    ~WiFiInitializer() {
    }
};

std::atomic<uint32_t> WiFi::m_ip;

void WiFi::init() {
    static WiFiInitializer init;
}

void WiFi::connect(const char* ssid, const char* pass) {
    init();

    esp_wifi_stop();

    if (gNetIf) {
        esp_netif_destroy(gNetIf);
    }

    gNetIf = esp_netif_create_default_wifi_sta();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t cfg = {};
    snprintf((char*)cfg.sta.ssid, 32, "%s", ssid);
    snprintf((char*)cfg.sta.password, 64, "%s", pass);
    esp_wifi_set_config(WIFI_IF_STA, &cfg);

    ESP_ERROR_CHECK(esp_wifi_start());
}

void WiFi::startAp(const char* ssid, const char* pass, uint8_t channel) {
    init();

    esp_wifi_stop();

    if (gNetIf) {
        esp_netif_destroy(gNetIf);
    }

    gNetIf = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t cfg = {};

    if (strlen(pass) >= 8) {
        snprintf((char*)cfg.ap.password, 64, "%s", pass);
        cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        ESP_LOGE(TAG, "The WiFi password is too short, 8 characters required, leaving the WiFI open!");
        cfg.ap.authmode = WIFI_AUTH_OPEN;
    }
    snprintf((char*)cfg.ap.ssid, 32, "%s", ssid);
    cfg.ap.channel = channel;
    cfg.ap.beacon_interval = 400;
    cfg.ap.max_connection = 4;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &cfg));

    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

void WiFi::eventHandler_netif(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        m_ip.store(0);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;

        char buf[16];
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "Got IP: %s\n",
            esp_ip4addr_ntoa(&event->ip_info.ip, buf, sizeof(buf)));
        m_ip.store(event->ip_info.ip.addr);
    }
}

}; // namespace rb

#endif
