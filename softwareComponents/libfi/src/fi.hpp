#pragma once
#include <iostream>
#include <functional>
#include <cstring>

#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>


#include <lwip/err.h>
#include <lwip/sys.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

namespace rofi::fi::detail {

enum WiFiBits {
    ConnectedBit = 1 << 0,
    FailedBit = 1 << 1,
    GotIPBit = 1 << 2
};

} // namespace rofi::fi::detail

namespace rofi::fi {

inline void initNvs() {
    esp_err_t ret = nvs_flash_init();
    if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
      ESP_ERROR_CHECK( nvs_flash_erase() );
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}

class WiFiConnector {
public:
    WiFiConnector() :
        _eventBits( xEventGroupCreate() )
    {
        _ipAddr.addr = 0;
    }

    ~WiFiConnector() {
        esp_event_handler_unregister( IP_EVENT, IP_EVENT_STA_GOT_IP,
            &WiFiConnector::eventHandler );
        esp_event_handler_unregister( WIFI_EVENT, ESP_EVENT_ANY_ID,
            &WiFiConnector::eventHandler );
        vEventGroupDelete( _eventBits );
    }

    /**
     * Connect asynchronously. The user supplies two callback - one on success
     * taking no parameters, one for error taking string message and the attempt
     * number. It return boolean specifying if the connection should be retried
     * or not.
     */
    WiFiConnector& async( std::function< void() > successCallback,
        std::function< void( esp_ip4_addr_t ) > ipCallback,
        std::function< bool( const std::string& , int ) > errorCallback )
    {
        _async = true;
        _successCallback = successCallback;
        _ipCallback = ipCallback;
        _errorCallback = errorCallback;
        return *this;
    }

    /**
     * Connect synchronously. The function block until connection is established
     * or the maximum number of retries was exceeded.
     */
    WiFiConnector& sync( int maxAttempts = 5 ) {
        _async = false;
        _maxAttempts = maxAttempts;
        _successCallback = [this]() {
            xEventGroupSetBits( _eventBits, detail::ConnectedBit );
        };
        _ipCallback = [this]( esp_ip4_addr_t ) {
            xEventGroupSetBits( _eventBits, detail::GotIPBit );
        };
        _errorCallback = [this]( const std::string&, int attempt ) {
            if ( attempt != _maxAttempts )
                return true;
            xEventGroupSetBits( _eventBits, detail::FailedBit );
            return false;
        };
        return *this;
    }

    /**
     * Connect to WiFi. `sync` or `async` should be called before calling this
     * method. If connection is performed synchronously, the return value of
     * this function determines the result. If the network has no password, pass
     * nullptr intstead of a password.
     */
    bool connect( const char *ssid, const char *password ) {
        _attemptNumber = 0;

        ESP_ERROR_CHECK( esp_netif_init() );
        ESP_ERROR_CHECK( esp_event_loop_create_default() );
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );

        ESP_ERROR_CHECK( esp_event_handler_register( WIFI_EVENT, ESP_EVENT_ANY_ID,
            &WiFiConnector::eventHandler, this ) );
        ESP_ERROR_CHECK( esp_event_handler_register( IP_EVENT, IP_EVENT_STA_GOT_IP,
            &WiFiConnector::eventHandler, this ) );

        wifi_config_t wifiConfig{};
        strcpy( reinterpret_cast< char * >( wifiConfig.sta.ssid ), ssid );
        if ( password )
            strcpy( reinterpret_cast< char * >( wifiConfig.sta.password ), password );
        else
            wifiConfig.sta.password[ 0 ] = '\0';
        wifiConfig.sta.threshold.authmode = password == nullptr ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
        wifiConfig.sta.pmf_cfg.capable = true;
        wifiConfig.sta.pmf_cfg.required = false;

        ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
        ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifiConfig ) );
        ESP_ERROR_CHECK( esp_wifi_start() );

        if ( _async )
            return true;

        EventBits_t bits = xEventGroupWaitBits( _eventBits,
            detail::ConnectedBit | detail::FailedBit,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY );
        if ( bits & detail::ConnectedBit )
            return true;
        if ( bits & detail::FailedBit )
            return false;
        __builtin_unreachable();
    }

    void waitForIp() {
        xEventGroupWaitBits( _eventBits, detail::GotIPBit, pdFALSE, pdFALSE,
            portMAX_DELAY );
    }

    std::string ipAddrStr() const {
        char buffer[ 32 ];
        return { esp_ip4addr_ntoa( &_ipAddr, buffer, 32 ) };
    }

private:
    static void eventHandler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data )
    {
        std::cout << "Event handler: " << event_base << ", " << arg << "\n";
        WiFiConnector &self = *reinterpret_cast< WiFiConnector *>( arg );

        if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START ) {
            std::cout << "Sta ready\n";
            esp_wifi_connect();
        }
        else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED ) {
            self._attemptNumber++;
            std::cout << "Failing\n";
            if ( self._errorCallback( "Connection failed", self._attemptNumber ) )
                esp_wifi_connect();
        }
        else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED ) {
            self._successCallback();
        }
        else if ( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP ) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            self._ipAddr = event->ip_info.ip;
            std::cout << "Sucess, we have IP!\n";
            self._ipCallback(self._ipAddr);
        }
    }

    int _attemptNumber = 0;
    int _maxAttempts = 0;
    bool _async;
    EventGroupHandle_t _eventBits;
    std::function< void() > _successCallback;
    std::function< bool( const std::string&, int ) > _errorCallback;
    std::function< void( esp_ip4_addr_t ) > _ipCallback;
    esp_ip4_addr_t _ipAddr;
};

inline void delayMs( int msCount ) {
    vTaskDelay( msCount / portTICK_PERIOD_MS );
}

/**
 * Perform authentication for wlan_fi
 */
void authWlanFi(const char* username, const char* password );

} // namespace rofi::fi
