#pragma once
#include <vector>
#include <cstdint>
#include <optional>
#include "../../include/serializable.hpp"

#include <lwip++.hpp>

struct MemoryReadResult
{
    bool success;
    bool requestRemoteRead;
    // Used when a remote read is required. Leave nullopt to send message to leader.
    std::optional< rofi::hal::Ip6Addr > readTarget;
    std::vector< uint8_t > rawData;

    template< SerializableOrTrivial T >
    T data()
    {
        if constexpr( std::is_base_of_v< Serializable, T > )
        {
            
            T var{};
            var.deserialize( rawData.data() );
            return var;
        }

        return as< T >( rawData.data() );
    }
};