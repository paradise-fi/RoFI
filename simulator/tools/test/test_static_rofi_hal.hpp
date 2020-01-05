#pragma once

#include "rofi_hal.hpp"

#include <type_traits>

static_assert( !std::is_default_constructible_v< rofi::hal::RoFI > );

static_assert( !std::is_copy_constructible_v< rofi::hal::RoFI > );
static_assert( !std::is_copy_assignable_v< rofi::hal::RoFI > );

static_assert( std::is_move_constructible_v< rofi::hal::RoFI > );
static_assert( std::is_move_assignable_v< rofi::hal::RoFI > );


static_assert( !std::is_constructible_v< rofi::hal::Joint > );
static_assert( std::is_copy_constructible_v< rofi::hal::Joint > );
static_assert( std::is_copy_assignable_v< rofi::hal::Joint > );
