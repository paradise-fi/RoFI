#pragma once

#include <atoms/test_aid.hpp>
#include <catch2/catch.hpp>
#include <fmt/format.h>

#include "configuration/serialization.hpp"


namespace Catch
{

template <>
struct StringMaker< rofi::configuration::matrices::Vector > {
    static std::string convert( const rofi::configuration::matrices::Vector & vec )
    {
        return fmt::format( "[{}, {}, {}, {}]", vec[ 0 ], vec[ 1 ], vec[ 2 ], vec[ 3 ] );
    }
};

template <>
struct StringMaker< nlohmann::json > {
    static std::string convert( const nlohmann::json & json )
    {
        return json.dump( 2 );
    }
};
template <>
struct StringMaker< rofi::configuration::RofiWorld > {
    static std::string convert( const rofi::configuration::RofiWorld & world )
    {
        using namespace rofi::configuration;
        auto attribCb = overload{
                []( const UniversalModule & mod ) {
                    return nlohmann::json::object( {
                            { "posA", matrices::center( mod.getBodyA().getPosition() ) },
                            { "posB", matrices::center( mod.getBodyB().getPosition() ) },
                    } );
                },
                []( auto &&... ) { return nlohmann::json{}; },
        };

        return rofi::configuration::serialization::toJSON( world, attribCb ).dump( 2 );
    }
};
} // namespace Catch
