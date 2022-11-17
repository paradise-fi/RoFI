#include "voxel_json_cmd.hpp"

#include <nlohmann/json.hpp>

#include "common_opts.hpp"
#include "voxel.hpp"


static std::filesystem::path inputCfgFilePath = {};
static std::filesystem::path outputCfgFilePath = {};
static bool reverse = {};

void voxelJsonSetOptions( Dim::Cli & cli )
{
    cli.opt( &inputCfgFilePath, "<input_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Input configuration file ('-' for standard input)" );
    cli.opt( &outputCfgFilePath, "<output_cfg_file>" )
            .defaultDesc( {} )
            .desc( "Output configuration file ('-' for standard output)" );

    cli.opt( &reverse, "r reverse." )
            .desc( "Convert from voxel-json to json configuration (default is opposite)" );
}

void toVoxelJsonCmd()
{
    auto inputCfg = readInput( inputCfgFilePath, readJsonCfgFromStream )
                            .get_or_throw_as< std::runtime_error >();
    assert( inputCfg );
    auto voxelJson = nlohmann::json( rofi::voxel::VoxelWorld::fromRofiWorld( *inputCfg )
                                             .get_or_throw_as< std::runtime_error >() );
    writeOutput( outputCfgFilePath, [ &voxelJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << voxelJson << std::endl;
    } );
}

void fromVoxelJsonCmd()
{
    auto inputCfg = readInput( inputCfgFilePath, []( std::istream & istr ) {
        return nlohmann::json::parse( istr ).get< rofi::voxel::VoxelWorld >();
    } );
    auto rofiWorld = inputCfg.toRofiWorld().get_or_throw_as< std::runtime_error >();
    assert( rofiWorld );
    auto worldJson = rofi::configuration::serialization::toJSON( *rofiWorld );
    writeOutput( outputCfgFilePath, [ &worldJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << worldJson << std::endl;
    } );
}

void voxelJsonCmd( Dim::Cli & /* cli */ )
{
    if ( reverse ) {
        fromVoxelJsonCmd();
    } else {
        toVoxelJsonCmd();
    }
}
