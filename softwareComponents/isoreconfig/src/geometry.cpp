#include <isoreconfig/geometry.hpp>

namespace rofi::isoreconfig {

Matrix pointToPos( const Point& point )
{
    Matrix result( arma::fill::eye );
    for ( int i = 0; i < 3; ++i )
        result(i,3) = point(i);
    return result;
}

Positions cloudToPositions( const Cloud& cop )
{
    Positions result;
    for ( const Point& p : cop ) 
        result.push_back( pointToPos( p ) );
    return result;
}

Point posToPoint( const Matrix& position )
{
    return Point{ position.at(0,3), position.at(1,3), position.at(2,3) };
}

Cloud positionsToCloud( const Positions& poss )
{
    Cloud result;
    for ( const Matrix& pos : poss ) 
        result.push_back( posToPoint( pos ) );
    return result;
}

Score cloudToScore( const Cloud& cop )
{
    arma::mat result( cop.size(), 3 );

    for ( size_t i = 0; i < cop.size(); ++i )  
        for ( size_t j = 0; j < 3; ++j )
            result(i, j) = cop[i](j);

    return result;
}

Cloud scoreToCloud( const Score& score )
{
    Cloud result;

    for ( size_t i = 0; i < score.n_rows; ++i )
        result.push_back( { score(i,0), score(i, 1), score(i, 2) } );

    return result;
}


Point centroid( const Cloud& cop )
{
    assert( cop.size() >= 1 );
    Point result = { 0, 0, 0 };
    
    for ( const Point& point : cop )
        result += point;

    double pointCount = double(cop.size());
    return result / Point( { pointCount, pointCount, pointCount } );
}

Matrix centroid( const Positions& positions )
{
    Cloud cop;
    for ( const Matrix& pos : positions )
        cop.push_back( posToPoint( pos ) );

    return pointToPos( centroid( cop ) );
}

double cubeNorm( const Point& vec )
{
    return vec( 0 ) * vec( 0 ) + vec( 1 ) * vec( 1 ) + vec( 2 ) * vec( 2 );
} 

std::array< Cloud, 2 > longestVectors( const Point& center, const Cloud& cop, const double epsilon )
{
    std::array< Cloud, 2 > result{ std::vector{center} };
    double longestNorm = 0;
    double sndLongestNorm = 0;

    for ( const Point& currPoint : cop )
    {
        double nextNorm = cubeNorm( currPoint - center );

        if ( nextNorm - longestNorm > epsilon )
        {
            // New longest vector, shift old longest to second longest
            result[1] = std::exchange( result[0], { currPoint } );
            sndLongestNorm = std::exchange( longestNorm, nextNorm );
        }
        else if ( std::abs( nextNorm - longestNorm ) <= epsilon )
            // Another longest vector
            result[0].push_back( currPoint );
        else if ( nextNorm - sndLongestNorm > epsilon )
        {
            // New second longest vector
            result[1] = { currPoint };  
            sndLongestNorm = nextNorm;
        }
        else if ( std::abs( nextNorm - sndLongestNorm ) <= epsilon )
            // Another second longest vector
            result[1].push_back( currPoint );         
    }

    return result;
}

std::array< Cloud, 2 > longestVectors( const Cloud& cop, const double eps )
{
    return longestVectors( centroid( cop ), cop, eps );
}

} // namespace rofi::isoreconfig
