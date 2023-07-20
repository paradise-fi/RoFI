#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"
#include <rofi_hal.hpp>
#include <optional>


using namespace rb;
using namespace gridui;

static Protocol* gProt = nullptr;
static Led gLedRed;

int jointPosition = -90;

// not working with localRoFI enabled - crashing
// auto localRoFI = rofi::hal::RoFI::getLocalRoFI();

static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
}

extern "C" void app_main() {
    auto localRoFI = rofi::hal::RoFI::getLocalRoFI();

    // Initialize WiFi
    WiFi::connect(SSID, PASSWORD);

    // Initialize RBProtocol
    gProt = new Protocol("FrantaFlinta", "Robocop", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);

    // Initialize the UI builder
    UI.begin(gProt);

    // Build the UI widgets
    gLedRed = UI.led(1, 1, 1, 1)
                  .color("red")
                  .on(true)
                  .finish();

    // The return values from finish() represent the constructed Widget in the UI.
    // They can be freely copied by value.
    auto boxBlack = UI.checkbox(4.5, 2.5, 4, 1)
                        .color("black")
                        .checked(true)
                        .text("ChkBox")
                        .finish();

    auto ledBlue = UI.led(3, 1, 1, 1)
                       .color("blue")
                       .finish();

    auto boxGreen = UI.checkbox(9.5, 2, 1.5, 3.5)
                        .color("green")
                        .fontSize(17.5)
                        .text("TestBox")
                        .onChanged([](Checkbox& b) {
                            printf("Checkbox changed: %d\n", (int)b.checked());
                        })
                        .finish();

    auto text = UI.text(0, 0, 10, 1)
                    .text("Hello, world!")
                    .finish();

    UI.joystick(5.5, 11.5, 5.5, 5)
        .color("red")
        .keys("wasd ")
        .text("Fire!")
        .onPositionChanged([](Joystick& joy) {
            const auto x = joy.x();
            const auto y = joy.x();
            if (x != 0 || y != 0) {
                printf("Joystick value: %d %d\n", x, y);
            }
        })
        .onClick([](Joystick&) {
            printf("Fire!\n");
        })
        .finish();

    auto button = UI.button(0, 11.5, 4.5, 1.5)
        .text("BUTTON")
        .css("border", "3px solid black")
        .css("text-transform", "uppercase")
        .onPress([&](Button&) {
            auto joint = localRoFI.getJoint(0);
            std::cout << "Joint position: " << joint.getTorque() << std::endl;
            // text.setText("Joint position: " + std::to_string(joint.getPosition()));
            // joint.setPosition( -1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone1\n"; } );
            // vTaskDelay( 2000 / portTICK_PERIOD_MS );
            // joint.setPosition( 1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone2\n"; } );
            // vTaskDelay( 2000 / portTICK_PERIOD_MS );
            // localRoFI->getJoint(0).setPosition(jointPosition, localRoFI->getJoint(0).maxSpeed(), [] ( auto ){});
            printf("Button pressed, servo position: %d\n", jointPosition);
            if(jointPosition == -90) {
                jointPosition = 90;
            } else {
                jointPosition = -90;
            }
        })
        .finish();

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    UI.commit();

    // Manipulating the created widgets:
    ledBlue.setColor("cyan");

    boxGreen.setText("Green!");

    boxBlack.setFontSize(20);

    while (true) {
            gLedRed.setOn(!gLedRed.on());
            sleep(1000);
    }
}