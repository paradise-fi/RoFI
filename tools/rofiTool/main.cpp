#include <dimcli/cli.h>
#include <iostream>
#include "preview.hpp"

int main( int argc, char * argv[] ) {
    Dim::Cli cli;
    cli.command("preview").action( preview );
    try {
        return cli.exec( std::cerr, argc, argv );
    }
    catch ( const std::runtime_error& e ) {
        std::cerr << "Error occured: " << e.what() << "\n";
        return 1;
    }
}