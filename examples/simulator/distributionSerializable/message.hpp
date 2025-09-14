#pragma once

#include <string>
#include "serializable/serializable.hpp"
#include <cstring>

/// @brief The serializable 'interface' allows us to send more complex structures over the network, provided we implement our own serialization/deserialization.
struct Message : public Serializable
{
    std::string message;

    Message() = default;
    Message( std::string message) : message( message ) {}

    /// @brief Copy the value of this structure into buffer. 
    /// @param buffer The buffer, it is the responsibility of the serialize() implementation to move the pointer by the size of your data type.
    virtual void serialize( uint8_t*& buffer ) const
    {
        std::memcpy(buffer, message.c_str(), message.size() + 1);
        buffer += message.size() + 1;
    };

    /// @brief Copy the value from the buffer into this structure.
    /// @param buffer The buffer, it is the responsibility of the deserialize() implementation to move the pointer by the size of your data type.
    virtual void deserialize( const uint8_t*& buffer ) 
    {
        int size = 0;
        while ( *( buffer + size ) != '\0' )
        {
            ++size;
        }

        message.resize( size );
        std::memcpy( message.data(), buffer, size );
    };

    /// @brief The size of your data type.
    /// @return size_t of the size of your data type.
    virtual std::size_t size() const
    {
        return message.size() + 1;
    }
};