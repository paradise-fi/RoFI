#include <iostream>

#include <dimcli/cli.h>

#include "common_opts.hpp"
#include "voxel_json_cmd.hpp"


std::filesystem::path UserConfig::inputCfgFileName = {};
std::filesystem::path UserConfig::outputCfgFileName = {};


int main( int argc, char * argv[] )
{
    Dim::Cli cli;

    cli.command( "voxel-json" )
            .desc( "Convert configuration to voxel json" )
            .action( voxelJsonCmd );

    cli.opt( &UserConfig::inputCfgFileName, "<input_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Input configuration file ('-' for standard input)" );
    cli.opt( &UserConfig::outputCfgFileName, "<output_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Output configuration file ('-' for standard output)" );

    return cli.exec( std::cerr, argc, argv );
}
