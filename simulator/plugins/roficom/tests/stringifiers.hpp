#include "roficomConnect.hpp"


std::string to_string( rofi::messages::ConnectorState::Orientation orientation )
{
    using ConnectorState = rofi::messages::ConnectorState;

    switch ( orientation )
    {
    case ConnectorState::NORTH:
        return "NORTH";
    case ConnectorState::SOUTH:
        return "SOUTH";
    case ConnectorState::EAST:
        return "EAST";
    case ConnectorState::WEST:
        return "WEST";
    default:
        throw std::runtime_error( "unknown connector orientation" );
    }
}

template< typename T >
std::string to_string( std::optional< T > opt )
{
    using std::to_string;
    using ::to_string; // Localy defined to_string should override std (for nice print of enums)
    using namespace std::string_literals;

    return opt ? "value( " + to_string( *opt ) + " )" : "nullopt"s;
}

template< typename T >
std::string fallback_stringifier( T value )
{
    using std::to_string;
    using ::to_string;

    return to_string( value );
}
