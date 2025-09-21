#include <vector>
#include <cstdint>
#include "../tasks/serializable/serializable.hpp"

struct MemoryReadResult
{
    bool success;
    std::vector< uint8_t > raw_data;

    template< SerializableOrTrivial T >
    T data()
    {
        if constexpr( std::is_base_of_v< Serializable, T > )
        {
            
            T var{};
            var.deserialize( raw_data.data() );
            return var;
        }

        return as< T >( raw_data.data() );
    }
};