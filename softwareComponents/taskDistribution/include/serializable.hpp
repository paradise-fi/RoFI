#pragma once

#include <type_traits>
#include <vector>
#include <cstdint>

struct Serializable {
    /// @brief Copy the value of this structure into buffer. 
    /// @param buffer The buffer, it is the responsibility of the serialize() implementation to move the pointer by the size of your data type.
    virtual void serialize( uint8_t*& buffer ) const = 0;

    /// @brief Copy the value from the buffer into this structure.
    /// @param buffer The buffer, it is the responsibility of the deserialize() implementation to move the pointer by the size of your data type.
    virtual void deserialize( const uint8_t*& buffer ) = 0;

    /// @brief The size of your data type.
    /// @return size_t of the size of your data type.
    virtual std::size_t size() const = 0;
};

template< typename T >
concept SerializableOrTrivial = std::is_base_of_v< Serializable, T > || std::is_trivially_copyable_v< T >;