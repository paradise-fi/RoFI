#define CATCH_CONFIG_MAIN
#include <type_traits>

#include <catch2/catch.hpp>

#include <rofi_hal.hpp>

static_assert( !std::is_default_constructible_v< rofi::hal::RoFI > );

static_assert( !std::is_copy_constructible_v< rofi::hal::RoFI > );
static_assert( !std::is_copy_assignable_v< rofi::hal::RoFI > );

static_assert( std::is_move_constructible_v< rofi::hal::RoFI > );
static_assert( std::is_move_assignable_v< rofi::hal::RoFI > );


static_assert( !std::is_constructible_v< rofi::hal::Joint > );
static_assert( std::is_copy_constructible_v< rofi::hal::Joint > );
static_assert( std::is_copy_assignable_v< rofi::hal::Joint > );
