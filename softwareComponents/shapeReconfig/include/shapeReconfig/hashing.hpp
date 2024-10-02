#include <configuration/rofiworld.hpp>

#include <shapeReconfig/geometry.hpp>

namespace rofi::shapereconfig {

size_t combineHash( size_t h1, size_t h2 );

struct RofiWorldHash
{
    size_t operator()( const rofi::configuration::RofiWorld& rw ) const;
};

template < typename ItemType, size_t ArrLen >
struct HashArray
{
    size_t operator()( const std::array< ItemType, ArrLen >& arr ) const
    {
        std::vector< size_t > tranArr( ArrLen );
        std::ranges::transform( arr, tranArr.begin(), std::hash< ItemType >{} );
        return std::accumulate( tranArr.begin(), tranArr.end(), size_t{}, combineHash );
    }
};

struct HashMat
{
    size_t operator()( const arma::mat& matr ) const
    {
        std::vector< size_t > tranArr( matr.n_elem );
        std::ranges::transform( matr, tranArr.begin(), std::hash< double >{} );
        return std::accumulate( tranArr.begin(), tranArr.end(), size_t{}, combineHash );
    }
};

struct HashCloud
{
    size_t operator()( const Cloud& cop ) const;
};

struct HashPairIntInt
{
    size_t operator()( const std::pair< int, int >& intPair ) const
    {
        return combineHash( std::hash<int>{}( intPair.first ), std::hash<int>{}( intPair.second ) );
    } 
};

} // namespace rofi::isoreconfig
