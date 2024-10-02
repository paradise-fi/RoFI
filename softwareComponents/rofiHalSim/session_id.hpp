#pragma once

#include <string>
#include <type_traits>


namespace rofi::hal
{

class SessionId {
public:
    static const SessionId & get();

    const std::string & bytes() const
    {
        return _bytes;
    }

private:
    template < typename T, std::enable_if_t< std::is_integral_v< T >, int > = 0 >
    constexpr SessionId( T id )
    {
        _bytes.reserve( sizeof( T ) );

        for ( size_t i = 0; i < sizeof( T ); i++ ) {
            _bytes.push_back( *( reinterpret_cast< char * >( &id ) + i ) );
        }
    }

    template < typename T,
               std::enable_if_t< sizeof( typename T::value_type ) == sizeof( char ), int > = 0 >
    constexpr SessionId( T id )
    {
        _bytes.reserve( id.size() );

        for ( auto & c : id ) {
            static_assert( sizeof( decltype( c ) ) == sizeof( char ) );
            _bytes.push_back( reinterpret_cast< char & >( c ) );
        }
    }

    SessionId( const SessionId & other ) = delete;
    SessionId & operator=( const SessionId & other ) = delete;

private:
    std::string _bytes;
};

} // namespace rofi::hal
