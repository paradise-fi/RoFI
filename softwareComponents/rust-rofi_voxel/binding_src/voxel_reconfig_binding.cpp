#include "voxel_reconfig.hpp"

#include <cstdint>

#include "configuration/serialization.hpp"


extern "C"
{
void rust_free_cstring( int8_t * rust_cstring );
int8_t * compute_reconfiguration_moves( const char * init_config_json,
                                        const char * goal_config_json );
}

namespace rofi::voxel
{
auto voxel_reconfig( const rofi::voxel::VoxelWorld & init, const rofi::voxel::VoxelWorld & goal )
        -> atoms::Result< std::vector< rofi::voxel::VoxelWorld > >
{
    using namespace std::string_literals;
    namespace serialization = rofi::configuration::serialization;

    auto initJson = nlohmann::json( init ).dump();
    auto goalJson = nlohmann::json( goal ).dump();

    auto resultPtr = compute_reconfiguration_moves( initJson.c_str(), goalJson.c_str() );
    if ( !resultPtr ) {
        return atoms::result_error( "Error inside Rust voxel reconfiguration (see stderr)"s );
    }

    auto resultJson = nlohmann::json();
    try {
        resultJson = nlohmann::json::parse( resultPtr );
    } catch ( const nlohmann::json::exception & e ) {
        rust_free_cstring( resultPtr );
        return atoms::result_error( "Error while parsing json from Rust voxel reconfiguration ("s
                                    + e.what() + ")" );
    }
    rust_free_cstring( resultPtr );

    try {
        return atoms::result_value( resultJson.get< std::vector< VoxelWorld > >() );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while getting VoxelWorlds from json ("s + e.what()
                                    + ")" );
    }

    return atoms::result_error( "Not implemented"s );
}

} // namespace rofi::voxel
