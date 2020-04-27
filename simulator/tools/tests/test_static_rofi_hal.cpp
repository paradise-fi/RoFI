#define CATCH_CONFIG_MAIN
#include <type_traits>

#include <catch2/catch.hpp>

#include <rofi_hal.hpp>

using namespace rofi::hal;

static_assert( !std::is_default_constructible_v< RoFI > );
static_assert( std::is_copy_constructible_v< RoFI > );
static_assert( std::is_copy_assignable_v< RoFI > );
static_assert( std::is_move_constructible_v< RoFI > );
static_assert( std::is_move_assignable_v< RoFI > );

static_assert( !std::is_constructible_v< Joint > );
static_assert( std::is_copy_constructible_v< Joint > );
static_assert( std::is_copy_assignable_v< Joint > );
static_assert( std::is_move_constructible_v< Joint > );
static_assert( std::is_move_assignable_v< Joint > );

static_assert( !std::is_constructible_v< Connector > );
static_assert( std::is_copy_constructible_v< Connector > );
static_assert( std::is_copy_assignable_v< Connector > );
static_assert( std::is_move_constructible_v< Connector > );
static_assert( std::is_move_assignable_v< Connector > );
