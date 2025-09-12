#pragma once

#include <type_traits>
#include <vector>
#include <cstdint>

struct Serializable {
    virtual void serialize( uint8_t*& buffer ) const = 0;
    virtual void deserialize( const uint8_t*& buffer ) = 0;
    virtual std::size_t size() const = 0;
};

template< typename T >
concept SerializableOrTrivial = std::is_base_of_v< Serializable, T > || std::is_trivially_copyable_v< T >;