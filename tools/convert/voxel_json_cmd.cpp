#include "voxel_json_cmd.hpp"

#include <nlohmann/json.hpp>

#include "common_opts.hpp"
#include "voxel.hpp"


static std::filesystem::path inputCfgFilePath = {};
static std::filesystem::path outputCfgFilePath = {};

void voxelJsonSetOptions( Dim::Cli & cli )
{
    cli.opt( &inputCfgFilePath, "<input_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Input configuration file ('-' for standard input)" );
    cli.opt( &outputCfgFilePath, "<output_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Output configuration file ('-' for standard output)" );
}

void voxelJsonCmd( Dim::Cli & /* cli */ )
{
    auto inputCfg = readInput( inputCfgFilePath, readJsonCfgFromStream )
                            .get_or_throw_as< std::runtime_error >();
    assert( inputCfg );
    auto voxelJson = nlohmann::json( rofi::voxel::VoxelWorld::fromRofiWorld( *inputCfg ) );
    writeOutput( outputCfgFilePath, [ &voxelJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << voxelJson << std::endl;
    } );
}
