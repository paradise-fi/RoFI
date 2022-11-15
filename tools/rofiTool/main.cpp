#include <dimcli/cli.h>
#include <iostream>
#include "commands.hpp"

int main( int argc, char * argv[] ) {
    Dim::Cli cli;
    cli.command( "points"  ).action( points  );
    cli.command( "preview" ).action( preview );
    cli.command( "check"   ).action( check   );
    cli.command( "build"   ).action( build   );
    try {
        return cli.exec( std::cerr, argc, argv );
    }
    catch ( const std::runtime_error& e ) {
        std::cerr << "Invalid configuration: " << e.what() << "\n";
        return 1;
    }
}

