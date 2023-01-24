#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_console.h>

#include <string_view>
#include <iostream>
#include <sstream>
#include <optional>

#include <atoms/util.hpp>
#include <atoms/units.hpp>

#include <rofi_hal.hpp>

using namespace std::literals;

// This is a simple utility, so let's make things simple and use global objects
// so we can reference it from plain C functions required by CLI interface
std::optional< rofi::hal::RoFI > localRoFI;
float speedCoef = 1;


// Simple wrapper that catches any exceptions, reports them an returns with a
// non-zero code if any error ocurred.
template < void (*Proc)( int argc, char** argv ) >
int handled( int argc, char** argv ) {
    try {
        Proc( argc, argv );
    }
    catch ( const std::exception& e ) {
        std::cerr << "Failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

void delay( int ms ) {
    vTaskDelay( ms / portTICK_PERIOD_MS );
}

void ensureArguments( int argc, int expected ) {
    if ( argc != expected + 1 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
}

void runMove( int argc, char **argv ) {
    ensureArguments( argc, 2 );
    static const std::array< std::string, 3 > servos = { "alpha", "beta", "gamma" };
    if ( !contains( servos, argv[ 1 ] ) )
        throw std::runtime_error( "Invalid servo name '"s + argv[ 1 ] + "'"s );
    int jointId = indexOf( argv[ 1 ], servos );

    std::istringstream s( argv[2] );
    float raw_position;
    if ( !( s >> raw_position ) )
        throw std::runtime_error( "Invalid position "s + argv[2] );
    auto position = Angle::deg( raw_position );

    auto joint = localRoFI->getJoint( jointId );
    float speed = speedCoef * joint.maxSpeed();
    if ( position.rad() > joint.maxPosition() || position.rad() < joint.minPosition() )
        throw std::runtime_error( "The requested position is out of range" );

    joint.setPosition( position.rad(), speed, [] ( auto ){} );
}

void registerMove() {
    const esp_console_cmd_t cmd = {
        .command = "move",
        .help = "Move a joint",
        .hint = "Arguments: joint targetPosition",
        .func = &handled< runMove >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runSetSpeed( int argc, char **argv ) {
    ensureArguments( argc, 1 );
    std::istringstream s( argv[ 1 ] );
    float coef;
    if ( ! ( s >> coef ) || coef < 0 || coef > 1  )
        throw std::runtime_error( "Invalid speed coefficient: "s + argv[ 1 ] );
    speedCoef = coef;
}

void registerSetSpeed() {
    const esp_console_cmd_t cmd = {
        .command = "setSpeed",
        .help = "Set speed coefficient 0-1",
        .hint = "Arguments: coefficient",
        .func = &handled< runMove >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

int readConnector( const char *str ) {
    std::istringstream s ( str );
    int connector;
    if ( ! ( s >> connector ) || connector < 1 || connector > 6  )
        throw std::runtime_error( "Invalid connector: "s + str );
    return connector - 1;
}

void runExpand( int argc, char **argv ) {
    ensureArguments( argc, 1 );
    auto connector = localRoFI->getConnector( readConnector( argv[ 1 ] ) );
    connector.connect();
}

void registerExpand() {
    const esp_console_cmd_t cmd = {
        .command = "expand",
        .help = "Expand a connector",
        .hint = "Arguments: connector",
        .func = &handled< runExpand >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runRetract( int argc, char **argv ) {
    ensureArguments( argc, 1 );
    auto connector = localRoFI->getConnector( readConnector( argv[ 1 ] ) );
    connector.disconnect();
}

void registerRetract() {
    const esp_console_cmd_t cmd = {
        .command = "retract",
        .help = "Retract a connector",
        .hint = "Arguments: connector",
        .func = &handled< runRetract >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}


void registerCommands() {
    registerMove();
    registerSetSpeed();
    registerExpand();
    registerRetract();
}

extern "C" void app_main() {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "umConsole>";

    localRoFI = rofi::hal::RoFI::getLocalRoFI();

    registerCommands();

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_console_new_repl_uart( &uart_config, &repl_config, &repl ) );
    ESP_ERROR_CHECK( esp_console_start_repl( repl ) );
}
