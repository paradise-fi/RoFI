#include <iostream>

#include <dimcli/cli.h>


int main( int argc, char * argv[] )
{
    return Dim::Cli().exec( std::cerr, argc, argv );
}
