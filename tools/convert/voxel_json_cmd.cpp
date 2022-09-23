#include "voxel_json_cmd.hpp"

#include <nlohmann/json.hpp>

#include "common_opts.hpp"
#include "voxel.hpp"


void voxelJsonCmd( Dim::Cli & /* cli */ )
{
    auto inputCfg = UserConfig::readInputCfg( readJsonCfgFromStream );
    assert( inputCfg );
    auto voxelJson = nlohmann::json( rofi::voxel::VoxelWorld::fromRofiWorld( *inputCfg ) );
    UserConfig::writeOutputCfg( [ &voxelJson ]( std::ostream & ostr ) {
        ostr.width( 4 );
        ostr << voxelJson << std::endl;
    } );
}
