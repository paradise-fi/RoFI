#include "fi.hpp"
#include "util.hpp"

#include <esp_http_client.h>
#include <esp_log.h>
#include <string>

/**
 * Aid for passing lambdas & functional objects into C callbacks tailored for
 * esp_http_client library. This is a plain function templated by functional
 * object type which will extract a pointer from user_data, cast it and call the
 * callback. Do not forget to set user_data of the client to the address of the
 * labmda.
 */
template < typename F >
int httpLibCallbackInvoker( esp_http_client_event_t *evt ) {
    F& function = *reinterpret_cast< F * >( evt->user_data );
    return function( evt );
}

/**
 * Builds a callback which reads only data out of http request and processes it
 * via f
 */
template < typename F >
auto makeHttpDataCallback( F f ) {
    return [=]( esp_http_client_event_t *evt ) {
        switch(evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                f( static_cast< char *>( evt->data ), evt->data_len );
                break;
            default:
                break;
        }
        return ESP_OK;
    };
}

std::vector< std::string > getWlanFiAuthLinks( const char *username, const char *password ) {
    std::vector< std::string > authLinks;

    auto extractLoginLink = [ &authLinks ]( std::string&& s ) {
        if ( s.find("loginx") == std::string::npos )
            return;
        auto start = s.find( "https://" );
        auto end = s.find( '"', start );
        auto url = s.substr( start, end - start );
        authLinks.push_back(url);
    };

    rofi::util::LineReader lineReader;
    lineReader.bind( extractLoginLink );

    auto httpCallback = makeHttpDataCallback( [&]( char *data, int len ) {
        lineReader.push( data, len );
    });

    esp_http_client_config_t config {};
    config.url = "https://fadmin.fi.muni.cz/auth/sit/wireless/login2.mpl";
    config.username = username;
    config.password = password;
    config.auth_type = HTTP_AUTH_TYPE_BASIC;
    config.event_handler = httpLibCallbackInvoker< decltype( httpCallback ) >;
    config.user_data = &httpCallback;

    auto client = esp_http_client_init( &config );
    ESP_ERROR_CHECK( esp_http_client_perform( client ) );
    esp_http_client_cleanup( client );

    return authLinks;
}

/**
 * Check if "good green" of wlan_fi auth is present in the line
 */
bool greenDetected( const std::string& s ) {
    return s.find( "#008000" ) != std::string::npos;
}

/**
 * Check if non-empty <text> tag is present in the line
 */
bool ipDetected( const std::string& s ) {
    auto textStart = s.find( "<text" );
    if (textStart == std::string::npos)
        return false;
    auto end = s.find( '>', textStart );
    return s.size() >= end + 2 && std::isdigit( s[ end + 1 ] );
}

void wlanFiFollowAuthLink( const std::string& link,
    const char *username, const char *password )
{
    // The response of wlan_fi is an SVG image. If auth was correct, green
    // object is present and a non-empty <text> tag is present. Check it.
    rofi::util::LineReader lineReader;
    bool gotGreen = false, gotText = false;
    lineReader.bind( [&]( std::string&& line ) {
        gotGreen |= greenDetected( line );
        gotText |= ipDetected( line );
    });
    auto httpCallback = makeHttpDataCallback( [&]( char *data, int len ) {
        lineReader.push( data, len );
    });

    esp_http_client_config_t config {};
    config.url = link.c_str();
    config.username = username;
    config.password = password;
    config.auth_type = HTTP_AUTH_TYPE_BASIC;
    config.user_data = &httpCallback;
    config.event_handler = httpLibCallbackInvoker< decltype( httpCallback ) >;

    esp_http_client_handle_t client  = esp_http_client_init( &config );
    ESP_ERROR_CHECK( esp_http_client_perform( client ) );
    int status = esp_http_client_get_status_code( client );
    esp_http_client_cleanup( client );

    if ( status != 200 || !gotGreen || !gotText )
        throw std::runtime_error( "Cannot authenticate to wlan_fi" );
}

void rofi::fi::authWlanFi(const char* username, const char* password ) {
    for( const auto& authLink : getWlanFiAuthLinks( username, password ) ) {
        if (authLink.find( "ip6" ) != std::string::npos )
            continue; // Ignore IPv6 address
        wlanFiFollowAuthLink( authLink, username, password );
    }
}
