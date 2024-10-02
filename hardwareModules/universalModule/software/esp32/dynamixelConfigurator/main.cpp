#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/uart.h>
#include <esp_console.h>

#include <string_view>
#include <iostream>
#include <sstream>

#include <fort.hpp>
#include <peripherals/dynamixel.hpp>
#include <atoms/util.hpp>

#include "enumIntrospection.hpp"

using namespace std::literals;
using namespace rofi::hal::dynamixel;

// This is a simple utility, so let's make things simple and use global bus
// object so we can reference it from plain C functions required by CLI
// interface
int gBaudrate = 57600;
Bus gBus = Bus( UART_NUM_1, GPIO_NUM_4, gBaudrate );


// Simple wrapper that catches any exceptions, reports them an returns with a
// non-zero code if any error ocurred.
template < void (*Proc)( int argc, char** argv ) >
int handled( int argc, char** argv ) {
    try {
        Proc( argc, argv );
    }
    catch ( const std::exception& e ) {
        std::cerr << "Failed: " << e.what() << "\n";
        // Restore baudrate in case we failed in the middle
        gBus.reconfigureBaudrate( gBaudrate );
        return 1;
    }
    return 0;
}

// Given a string, parse ID (either decimal or hexadecimal). If ID is invalid,
// throw an exception
Id readId( const char * str ) {
    std::istringstream s( str );
    int id;
    if ( ! ( s >> id ) )
        throw std::runtime_error( "Invalid ID: "s + str );
    if ( id < 0 || id > 0xFE )
        throw std::runtime_error( "Invalid ID range: "s + str );
    return Id( id );
}

// Read a response from gBus and validate it. If invalid, throw an exception
Packet getReponse( int tickTimeout = 1000 / portTICK_PERIOD_MS ) {
    auto p = gBus.read( tickTimeout );
    if ( !p.has_value() ) {
        throw std::runtime_error( "No response in time" );
    }
    Packet packet = *p;
    if ( !p->valid() )
        throw std::runtime_error( "Invalid response" );
    if ( p->error() != Error::Nothing )
        throw std::runtime_error( "Response contains error: "s
            + to_string( Error( p->error() ) ) );
    return *p;
}

// Scan bus for servos and invoke callback on them. If the callback returns
// false, search stops.
template < typename F >
void scanServos( F f ) {
    for ( int baudrate : { 57600, 115200, 1000000, 2000000, 3000000, 4500000} ) {
        gBus.reconfigureBaudrate( baudrate );

        gBus.send( Packet::ping( 0xFE ) );
        std::optional< Packet > p = gBus.read( 20 );
        bool end = false;
        while ( p.has_value() ) {
            end = !f( p->id(), baudrate );
            if ( end )
                break;
            p = gBus.read( 20, false );
        }
        if ( end )
            break;
    }
    gBus.reconfigureBaudrate( gBaudrate );
}

void runDumpRegisters( int argc, char **argv ) {
    if ( argc != 2 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    Id id = readId( argv[ 1 ] );
    fort::char_table romTable, ramTable;
    romTable << fort::header
             << "ROM Register" << "Raw value" << "Interpreted value" << fort::endr;
    ramTable << fort::header
             << "RAM Register" << "Raw value" << "Interpreted value" << fort::endr;

    for ( Register r: RegisterValues ) {
        gBus.send( Packet::read( id, r ) );
        Packet response = getReponse();
        int value;
        switch ( registerSize( r ) ) {
            case 1:
                value = response.get< uint8_t >();
                break;
            case 2:
                value = response.get< uint16_t >();
                break;
            case 4:
                value = response.get< uint32_t >();
                break;
            default:
                assert( false );
        }

        auto& table = r >= Register::TorqueEnable
                        ? ramTable
                        : romTable;

        table << to_string( r ) << value
              << interpretRegisterValue( r, value ) << fort::endr;
    }

    std::cout << romTable.to_string() << "\n\n" << ramTable.to_string() << "\n";
}

void registerDumpRegisters() {
    const esp_console_cmd_t cmd = {
        .command = "dumpRegisters",
        .help = "Dump all registers in the servo",
        .hint = "Arguments: ID",
        .func = &handled< runDumpRegisters >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runScan( int argc, char **argv ) {
    for ( int baudrate : { 9600, 57600, 115200, 1000000, 2000000, 3000000, 4500000} ) {
        gBus.reconfigureBaudrate( baudrate );
        uint32_t baud = 0;

        gBus.send( Packet::ping( 0xFE ) );
        std::optional< Packet > p = gBus.read( 20 );
        while ( p.has_value() ) {
            std::cout << "Found ID: " << int( p->id() ) << "    (" << baudrate << " bps)\n";
            p = gBus.read( 20, false );
        }
    }
    gBus.reconfigureBaudrate( gBaudrate );
}

void registerScan() {
    const esp_console_cmd_t cmd = {
        .command = "scan",
        .help = "Scan bus for attached servos",
        .hint = "No arguments",
        .func = &handled< runScan >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runSetRegister( int argc, char** argv ) {
    if ( argc != 4 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    Id id = readId( argv[ 1 ] );
    Register reg = readRegisterName( argv[ 2 ] );
    int value = std::stoi( argv[ 3 ] );
    int regSize = registerSize( reg );

    std::optional< Packet > p;
    switch ( registerSize( reg ) ) {
        case 1:
            p = Packet::write( id, reg, uint8_t( value ) );
            break;
        case 2:
            p = Packet::write( id, reg, int16_t( value ) );
            break;
        case 4:
            p = Packet::write( id, reg, value );
            break;
        default:
            assert( false );
    }
    gBus.send( *p );
    auto response = getReponse();
    std::cout << "OK\n";
}

void registerSetRegister() {
    const esp_console_cmd_t cmd = {
        .command = "setRegister",
        .help = "Write to register",
        .hint = "Arguments: ID RegisterName Value",
        .func = &handled< runSetRegister >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runGetBaudrate( int argc, char **argv ) {
    std::cout << "Baudrate " << gBaudrate << "\n";
}

void registerGetBaudrate() {
    const esp_console_cmd_t cmd = {
        .command = "getBaudrate",
        .help = "Get current bus baudrate",
        .hint = "",
        .func = &handled< runGetBaudrate >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runSetBaudrate( int argc, char **argv ) {
    if ( argc != 2 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    std::istringstream s( argv[ 1 ] );
    int baudrate;
    char suffix = '\0';
    s >> baudrate >> suffix;
    if ( suffix == 'M' )
        baudrate *= 1000000;
    gBus.reconfigureBaudrate( baudrate );
    gBaudrate = baudrate;
    std::cout << "Baudrate changed to " << baudrate << "\n";
}

void registerSetBaudrate() {
    const esp_console_cmd_t cmd = {
        .command = "setBaudrate",
        .help = "Set current bus baudrate",
        .hint = "",
        .func = &handled< runSetBaudrate >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runMakeServo( int argc, char **argv ) {
    if ( argc != 2 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    static const std::array< std::string, 3 > servos = { "alpha", "beta", "gamma" };
    if ( !contains( servos, argv[ 1 ] ) )
        throw std::runtime_error( "Invalid servo name '"s + argv[ 1 ] + "'"s );
    Id newId = Id( indexOf( argv[ 1 ], servos ) ) + 1;

    std::cout << "Scanning servos... ";
    std::vector< std::tuple< Id, int > > availableServos;
    scanServos( [&]( Id id, int baudrate ) {
        availableServos.emplace_back( id, baudrate );
        return true;
    } );

    if ( availableServos.empty() ) {
        std::cout << "\n";
        throw std::runtime_error( "No servos found" );
    }

    auto selectedServo = availableServos[ 0 ];
    if ( availableServos.size() > 1 ) {
        std::cout << "Found " << availableServos.size() << " servos\n";
        std::cout << "Select one of the following:\n";
        int idx = 1;
        for ( auto [id, baudrate] : availableServos ) {
            std::cout << idx << ": ID " << int( id ) << " @ " << baudrate << "bps\n";
            idx++;
        }
        int choice;
        std::cout << "Your choice: ";
        std::cin >> choice;
        if ( choice < 1 || choice > availableServos.size() )
            throw std::runtime_error( "Invalid choice" );
        selectedServo = availableServos[ choice - 1 ];

    } else {
        std::cout << "Found 1 servo.\n";
    }

    auto [id, baudrate] = selectedServo;
    std::cout << "Using servo ID " << int( id ) << " @ " << baudrate << "bps\n\n";

    gBus.reconfigureBaudrate( baudrate );

    std::cout << "Disabling torque... ";
    gBus.send( Packet::write( id, Register::TorqueEnable, uint8_t( 0 ) ) );
    getReponse();
    std::cout << "Done\n";

    std::cout << "Setting operating mode... ";
    gBus.send( Packet::write( id, Register::OperatingMode, uint8_t( 4 ) ) );
    getReponse();
    std::cout << "Done\n";

    std::cout << "Setting homing offset... ";
    gBus.send( Packet::write( id, Register::HomingOffset, int( 0 ) ) );
    getReponse();
    std::cout << "Done\n";

    std::cout << "Setting velocity limit... ";
    gBus.send( Packet::write( id, Register::VelocityLimit, int( 1023 ) ) );
    getReponse();
    std::cout << "Done\n";

    std::cout << "Setting new ID... ";
    gBus.send( Packet::write( id, Register::ID, uint8_t( newId ) ) );
    getReponse();
    std::cout << "Done\n";

    gBus.send( Packet::write( newId, Register::BaudRate, uint8_t( 3 ) ) );
    std::cout << "Done\n";

    gBus.reconfigureBaudrate( gBaudrate );
}

void registerMakeServo() {
    const esp_console_cmd_t cmd = {
        .command = "makeServo",
        .help = "Set current bus baudrate",
        .hint = "Arugments [alpha|beta|gamma]",
        .func = &handled< runMakeServo >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runFactoryReset( int argc, char **argv ) {
    if ( argc != 2 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    Id id = readId( argv[ 1 ] );

    gBus.send( Packet::factoryReset( id, 0XFF ) );
    getReponse();
}

void registerFactoryReset() {
    const esp_console_cmd_t cmd = {
        .command = "factoryReset",
        .help = "Perform full factory reset",
        .hint = "Arugments ID",
        .func = &handled< runFactoryReset >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}

void runGlobalFactoryReset( int argc, char **argv ) {
    for ( int baudrate : { 9600, 57600, 115200, 1000000, 2000000, 3000000, 4500000} ) {
        gBus.reconfigureBaudrate( baudrate );
        std::cout << "Trying baudrate: " << baudrate << "\n";
        gBus.send( Packet::factoryReset( 0xFE, 0XFF ) );
        vTaskDelay( 500 / portTICK_PERIOD_MS );
    }
    gBus.reconfigureBaudrate( gBaudrate );
}

void registerGlobalFactoryReset() {
    const esp_console_cmd_t cmd = {
        .command = "globalFactoryReset",
        .help = "Perform full factory reset",
        .hint = "",
        .func = &handled< runGlobalFactoryReset >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}


void runReboot( int argc, char **argv ) {
    if ( argc != 2 )
        throw std::runtime_error( "Invalid number of arguments " + std::to_string( argc ) );
    Id id = readId( argv[ 1 ] );
    auto servo = gBus.getServo( id );
    servo.reboot();
}

void registerReboot() {
    const esp_console_cmd_t cmd = {
        .command = "reboot",
        .help = "Reboot servo",
        .hint = "Arugments ID",
        .func = &handled< runReboot >,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK( esp_console_cmd_register( &cmd ) );
}


void registerCommands() {
    registerDumpRegisters();
    registerScan();
    registerSetRegister();
    registerGetBaudrate();
    registerSetBaudrate();
    registerMakeServo();
    registerFactoryReset();
    registerGlobalFactoryReset();
    registerReboot();
}

void delay( int ms ) {
    vTaskDelay( ms / portTICK_PERIOD_MS );
}


extern "C" void app_main() {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "dynamixelConfig>";

    registerCommands();

    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_console_new_repl_uart( &uart_config, &repl_config, &repl ) );
    ESP_ERROR_CHECK( esp_console_start_repl( repl ) );
}
