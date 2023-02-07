#pragma once

#include <atoms/test_aid.hpp>
#include <catch2/catch.hpp>

#include "voxel/serialization.hpp"


namespace Catch
{
template <>
struct StringMaker< rofi::voxel::VoxelWorld > {
    static std::string convert( const rofi::voxel::VoxelWorld & world )
    {
        return nlohmann::json( world ).dump( 2 );
    }
};
} // namespace Catch
