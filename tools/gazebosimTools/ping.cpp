#include <iostream>
#include <sstream>

#include "rofi_hal.hpp"


using namespace rofi::hal;


int main( int argc, char ** argv )
{
    std::cout << "Starting ping tool\n";

    if ( argc == 1 )
    {
        std::cerr << "Run:\n" << argv[ 0 ] << " [RoFI_ID] [...]\n";
        return 1;
    }

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
