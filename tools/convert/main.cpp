#include <iostream>

#include <dimcli/cli.h>

#include "common_opts.hpp"
#include "voxel_json_cmd.hpp"


int main( int argc, char * argv[] )
{
    Dim::Cli cli;

    cli.command( "voxel-json" )
            .desc( "Convert configuration to voxel json" )
            .action( voxelJsonCmd );
    voxelJsonSetOptions( cli );

    return cli.exec( std::cerr, argc, argv );
}
