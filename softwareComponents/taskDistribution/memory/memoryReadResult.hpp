#include <vector>
#include <cstdint>
#include "../tasks/serializable/serializable.hpp"

struct MemoryReadResult
{
    bool success;
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