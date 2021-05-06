#include <algorithm>
#include <iostream>
#include <sstream>
#include <string_view>

#include "rofi_hal.hpp"


using namespace rofi::hal;


int main( int argc, char ** argv )
{
    if ( argc == 1 || std::any_of( argv + 1, argv + argc, []( auto arg ) {
             using namespace std::string_view_literals;
             return arg == "-h"sv || arg == "--help"sv;
         } ) )
    {
        std::cerr << "Run:\n" << argv[ 0 ] << " [RoFI_ID ...]\n";
        return 1;
    }

    std::cout << "Starting ping tool\n";

    for ( int i = 1; i < argc; i++ )
    {
        auto strStream = std::stringstream( std::string( argv[ i ] ) );
        int rofiId;
        strStream >> rofiId >> std::ws;

        if ( !strStream || !strStream.eof() )
        {
            std::cerr << "Could not read RoFI ID" << std::endl;
            continue;
        }

        rofi::hal::RoFI::getRemoteRoFI( rofiId );
    }

    std::cout << "Ending ping tool\n";
}
