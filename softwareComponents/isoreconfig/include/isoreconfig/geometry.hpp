#include <array>
#include <cassert>
#include <vector>

#include <armadillo>

namespace rofi::isoreconfig {

static constexpr double ERROR_MARGIN = 0.01;

/**
 * @brief Cloud of Points (CoP) - normalized container of points
 * (transformed using PCA, rounded to int, sorted).
 */
class Cloud;

/**
 * @brief Decides whether given clouds define an equal physical shape.
 * Assumes different number of points in cloud means different shapes.
 * Attempts to find an orthogonal rotation
 * which transforms one cloud into the other.
 */
bool isometric( const Cloud& cop1, Cloud cop2 );

class Cloud
{
    using Point = std::array< int, 3 >;

    std::vector< Point > _points;
    arma::mat _coeff; // PC coefficients - cols are (base) eigenvectors

public:

    explicit Cloud( const std::vector< Vector >& pts ) 
    {
        arma::mat data;
        data.set_size( pts.size(), 3 );

        for ( size_t i = 0; i < pts.size(); ++i )
            for ( size_t j = 0; j < 3; ++j )
                data(i, j) = pts[i](j);

        std::tie( data, _coeff ) = normalize( data );
        auto rawPoints = dataToPoints( data );
        _points = roundPoints( rawPoints );
        sortPoints();
    }

    /**
     * @brief Rotates the points by 90 degrees around given axis.
     */
    void rotateBy90Around( size_t axis )
    {
        swapAxes( (axis + 1) % 3, (axis + 2) % 3 );
        negateAxis( (axis + 2) % 3 );
        sortPoints();
    }

    /**
     * @brief Rotates the points by 180 degrees around given axis.
     */
    void rotateBy180Around( size_t axis )
    {
        negateAxis( (axis + 1) % 3 );
        negateAxis( (axis + 2) % 3 );
        sortPoints();
    }

    /**
     * @brief Permutate axes (X -> Y -> Z -> X).
     */
    void permutateAxes()
    {
        swapAxes( 1, 2 );
        swapAxes( 0, 1 );
        sortPoints();
    }

    /**
     * @brief Returns the PCA transformation used to convert original points
     * into the points defining the resulting cloud.
     */
    arma::mat transformation() const
    {
        // transposed _coeff is PCA transformation matrix
        return _coeff.t();
    }

    /**
     * @brief Cloud points as vector< Vector >.
     */
    Vector toVectors() const
    {
        std::vector< Vector > vecs( size() );

        for ( size_t pt = 0; pt < size(); ++pt )
            for ( size_t col = 0; col < 3; ++col )
                vecs[pt](col) = _points[pt][col] * ERROR_MARGIN;

        return vecs;
    }

    bool operator==( const Cloud& o ) const
    {
        return _points == o._points;
    }

    size_t size() const
    {
        return _points.size();
    }

    auto begin() const
    {
        return _points.begin();
    }

    auto begin()
    {
        return _points.begin();
    }

    auto end() const
    {
        return _points.end();
    }

    auto end()
    {
        return _points.end();
    }

    void print() const
    {
        for ( const Point& pt : _points )
            std::cout << pt[0] << "   " << pt[1] << "   " << pt[2] << "\n";
    }

private:

    /**
     * @brief Normalize given points (data) to use their PCA coordinate system
     * (or its reflection in case the PCA transformation is a reflection,
     * so the shape given by the points does not change).
     * Returns normalized data and its transformation matrix.
     */
    std::tuple< arma::mat, arma::mat > normalize( const arma::mat& data ) const
    {
        arma::mat coeff;
        arma::mat score;
        arma::vec latent;
        arma::vec tsquared;

        princomp( coeff, score, latent, tsquared, data );
        auto determinant = det( coeff );
        assert( std::abs( determinant ) - 1 < ERROR_MARGIN );

        // If determinant is negative (therefore ~= -1), reflect along one plane (we use YZ).
        // (negative sign -> reflection, which can change the shape of the cloud)
        if ( determinant < 0 )
        {
            score.col(0) *= -1;
            coeff.col(0) *= -1;
        } 

        return std::tie( score, coeff );
    }

    std::vector< std::array< double, 3 > > dataToPoints( const arma::mat& data ) const
    {
        std::vector< std::array< double, 3 > > points( data.n_rows );
        
        for ( size_t pt = 0; pt < data.n_rows; ++pt )
            for ( size_t col = 0; col < 3; ++col )
                points[pt][col] = data( pt, col ); 
    
        return points;
    }

    std::vector< Point > roundPoints( 
        const std::vector< std::array< double, 3 > >& rawPoints ) const
    {   
        std::vector< Point > roundedPoints( rawPoints.size() );

        for ( size_t pt = 0; pt < rawPoints.size(); ++pt )
            for ( size_t col = 0; col < 3; ++col )
                roundedPoints[pt][col] = static_cast<int>( round( rawPoints[pt][col] / ERROR_MARGIN ) );

        return roundedPoints;
    }

    void sortPoints()
    {
        auto pointComparator = []( const Point& p1, const Point& p2 ) 
        {
            return p1[0] < p2[0]
                || ((p1[0] == p2[0] && p1[1] < p2[1]) 
                || (p1[0] == p2[0] && p1[1] == p2[1] && p1[2] < p2[2])); 
        };

        std::ranges::sort( _points, pointComparator );
    }

    void swapAxes( size_t ax1, size_t ax2 )
    {
        for ( Point& pt : _points )
            std::swap( pt[ax1], pt[ax2] );
    }

    void negateAxis( size_t ax )
    {
        for ( Point& pt : _points )
            pt[ax] = -pt[ax];
    }
};

} // namespace rofi::isoreconfig
