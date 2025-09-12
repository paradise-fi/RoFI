#include <type_traits>
#include <vector>
#include <cstdint>

struct Serializable {
    virtual void Serialize( uint8_t*& buffer ) const = 0;
    virtual void Deserialize( const uint8_t*& buffer ) = 0;
};

template< typename T >
concept SerializableOrTrivial = std::is_base_of_v< Serializable, T > || std::is_trivially_copyable_v< T >;