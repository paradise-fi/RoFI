#include "voxel_reconfig.hpp"

#include <cstdint>
#include <ranges>

#include "configuration/serialization.hpp"
#include "cpp_json_bindings.rs.h"


namespace rofi::voxel
{
auto voxel_reconfig( const rofi::voxel::VoxelWorld & init, const rofi::voxel::VoxelWorld & goal )
        -> atoms::Result< std::vector< rofi::voxel::VoxelWorld > >
{
    using namespace std::string_literals;
    namespace serialization = rofi::configuration::serialization;

    auto initJson = nlohmann::json( init ).dump();
    auto goalJson = nlohmann::json( goal ).dump();

    auto result = std::vector< rofi::voxel::VoxelWorld >();

    try {
        auto rustResult = voxel_reconfiguration( initJson, goalJson );
        std::ranges::copy( rustResult, std::back_insert_iterator( rustResult ) );
    } catch ( const nlohmann::json::exception & e ) {
        return atoms::result_error( "Error while parsing json from Rust voxel reconfiguration ("s
                                    + e.what() + ")" );
    }

    // auto resultJson = nlohmann::json();
    // try {
    //     resultJson = nlohmann::json::parse( resultPtr );
    // } catch ( const nlohmann::json::exception & e ) {
    //     rust_free_cstring( resultPtr );
    //     return atoms::result_error( "Error while parsing json from Rust voxel reconfiguration ("s
    //                                 + e.what() + ")" );
    // }
    // rust_free_cstring( resultPtr );

    // try {
    //     return atoms::result_value( resultJson.get< std::vector< VoxelWorld > >() );
    // } catch ( const nlohmann::json::exception & e ) {
    //     return atoms::result_error( "Error while getting VoxelWorlds from json ("s + e.what()
    //                                 + ")" );
    // }

    // return atoms::result_error( "Not implemented"s );
    return atoms::result_value( result );
}

} // namespace rofi::voxel
