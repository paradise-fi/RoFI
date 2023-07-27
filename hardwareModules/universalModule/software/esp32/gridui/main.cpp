#include <iostream>
#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include <fi.hpp>
#include <esp_log.h>
#include <rofi_hal.hpp>
#include <optional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

using namespace rb;
using namespace gridui;

// You can include layout.hpp in many .cpp files,
// but ONE of those must have this define before it.
#define GRIDUI_LAYOUT_DEFINITION
#include "layout.hpp"

static Protocol* gProt = nullptr;

static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
}

extern "C" void app_main() {

    esp_log_level_set("esp_netif_lwip", ESP_LOG_VERBOSE);

    std::cout << "Program starts\n";
    rofi::fi::initNvs();
    rofi::fi::WiFiConnector c;
    if ( c.sync( 5 ).connect(SSID, PASSWORD) )
        std::cout << "Connected: " << c.ipAddrStr() << "\n";
    else
        std::cout << "Connection failed\n";
    c.waitForIp();
    std::cout << "Got IP: " << c.ipAddrStr() << "\n";

    // esp_wifi_set_ps (WIFI_PS_NONE);

    if ( std::string( SSID ) == "wlan_fi" )
        rofi::fi::authWlanFi( WLAN_USER, WLAN_PASS );
    std::cout << "Autenticated!\n";

    // Initialize RBProtocol
    gProt = new Protocol("RoFI", "Robot", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);

    // Initialize the UI builder
    UI.begin(gProt);

    // Build the UI widgets. Positions/props are set in the layout, so most of the time,
    // you should only set the event handlers here.
    auto builder = Layout.begin();
    auto localRoFI = rofi::hal::RoFI::getLocalRoFI();

    builder.SliderJoint0
        .onChanged([&](Slider& s) {
            printf("Slider0 changed: %g\n", s.value());
            localRoFI.getJoint(0).setPosition(s.value(), localRoFI.getJoint(0).maxSpeed(), [] ( auto ){});
        });

    builder.SliderJoint1
        .onChanged([&](Slider& s) {
            printf("Slider1 changed: %g\n", s.value());
            localRoFI.getJoint(1).setPosition(s.value(), localRoFI.getJoint(1).maxSpeed(), [] ( auto ){});
        });

    builder.SliderJoint2
        .onChanged([&](Slider& s) {
            printf("Slider2 changed: %g\n", s.value());
            localRoFI.getJoint(2).setPosition(s.value(), localRoFI.getJoint(1).maxSpeed(), [] ( auto ){});
        });

    builder.commit();


    Layout.LedActive.setColor("green");
    Layout.TextId.setText(std::to_string(localRoFI.getId()));

    while (true) {
        Layout.LedActive.setOn(!Layout.LedActive.on());
        std::cout << "Led state: " << Layout.LedActive.on() << std::endl;
        if(gProt->is_possessed()) {
            gProt->send_log("Led state: %d", Layout.LedActive.on());
        }
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}