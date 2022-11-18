#pragma once

#include <string>

#include <catch2/catch.hpp>

#include "atoms/result.hpp"
#include "atoms/units.hpp"


namespace Catch
{
template < typename ValueT, typename ErrorT >
struct StringMaker< atoms::Result< ValueT, ErrorT > > {
    static std::string convert( const atoms::Result< ValueT, ErrorT > & result )
    {
        using namespace std::string_literals;
        return result.match( overload{
                []( std::true_type, const auto & value ) {
                    return "Value: "s + StringMaker< ValueT >::convert( value );
                },
                []( std::false_type, const auto & error ) {
                    return "Error: "s + StringMaker< ErrorT >::convert( error );
                },
        } );
    }
};

template <>
struct StringMaker< Angle > {
    static std::string convert( const Angle & angle )
    {
        return std::to_string( angle.deg() ) + " deg";
    }
};

} // namespace Catch
