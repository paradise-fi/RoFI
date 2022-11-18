#pragma once

#include <vector>

#include "atoms/result.hpp"
#include "voxel.hpp"

namespace rofi::voxel
{
auto voxel_reconfig( const rofi::voxel::VoxelWorld & init, const rofi::voxel::VoxelWorld & goal )
        -> atoms::Result< std::vector< rofi::voxel::VoxelWorld > >;
}
