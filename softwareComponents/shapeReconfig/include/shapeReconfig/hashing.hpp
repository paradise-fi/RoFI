#include <configuration/rofiworld.hpp>

#include <shapeReconfig/geometry.hpp>

namespace rofi::shapereconfig {

size_t combineHash( size_t h1, size_t h2 );

struct RofiWorldHash
{
    size_t operator()( const rofi::configuration::RofiWorld& rw ) const;
};

template < typename _Tp, size_t _Nm >
struct HashArray
{
    size_t operator()( const std::array< _Tp, _Nm >& arr ) const
    {
        size_t h = 0;
        for ( const _Tp& x : arr )
            h = combineHash( h, std::hash< _Tp >{}( x ) );
        return h;
    }
};

struct HashMat
{
    size_t operator()( const arma::mat& matr ) const
    {
        size_t h = 0;
        for ( double x : matr )
            h = combineHash( h, std::hash< double >{}( x ) );
        return h;
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
        return combineHash( std::hash< int >{}( intPair.first ), std::hash< int >{}( intPair.second ) );
    } 
};



/* struct HashNode
{
    size_t operator()( const Node& n ) const
    {
        size_t h = std::hash< size_t >{}( n.nid );
        h = combineHash( h, RofiWorldHash{}( n.world ) );
        h = combineHash( h, HashCloud{}( n.shape ) );
        return combineHash( h, n.distFromStart );
    }
}; */

} // namespace rofi::shapereconfig
