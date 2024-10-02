#pragma once

#include <array>
#include <cassert>
#include <vector>
#include <configuration/Matrix.h>

#include <armadillo>

namespace rofi::shapereconfig {

static constexpr double ERROR_MARGIN = 0.001;

using Vector = arma::vec4;

/**
 * @brief Cloud of Points (CoP) - normalized container of points
 * (transformed using PCA, rounded to int, sorted).
 */
class Cloud;

/**
 * @brief Calculate the centroid (unweighted average) of given points.
 * Assumes the container is not empty.
 * @param pts Points to calculate the center of gravity from.
 * @return Centroid of given points.
 */
Vector centroid( const std::vector< Vector >& pts );

/**
 * @brief Decides whether given clouds define an equal physical shape.
 * Assumes different number of points in cloud means different shapes.
 * Attempts to find an orthogonal rotation
 * which transforms one cloud into the other.
 */
bool isometric( const Cloud& cop1, Cloud cop2 );

/**
 * @brief Generates a container of 24 Clouds which have the same shape as the given Cloud.
 * Each cloud is an orthogonal rotation of the given cloud.
 */
std::vector< Cloud > sameShapeClouds( Cloud cop );

/**
 * @brief Canonical representation of all clouds with the same shape.
 * Used for hashing a specific shape.
 */
Cloud canonCloud( Cloud cop );

class Cloud
{
    using Point = std::array< int, 3 >;

    // Sphere is a vector of points with the same distance from the centroid of the whole Cloud
    std::vector< std::pair< size_t, std::vector< Point > > > _spheres;
    arma::mat _coeff; // PC coefficients - cols are (base) eigenvectors
    Vector _eigenValues;

public:

    Cloud() = default; // placeholder empty cloud

    explicit Cloud( const std::vector< Vector >& pts ) 
    {
        assert( !pts.empty() );

        arma::mat data;
        arma::mat latent;
        data.set_size( pts.size(), 3 );

        for ( size_t i = 0; i < pts.size(); ++i )
            for ( size_t j = 0; j < 3; ++j )
                data(i, j) = pts[i](j);

        std::tie( data, _coeff, latent ) = normalize( data );
        _eigenValues = { latent(0), latent(1), latent(2), 1 };

        _spheres = dataToSpherePoints( data );
        sortSpherePoints();
        sortSpheres();
    }

    /**
     * @brief Rotates the points by 90 degrees around given axis.
     */
    void rotateBy90Around( size_t axis )
    {
        swapAxes( (axis + 1) % 3, (axis + 2) % 3 );
        negateAxis( (axis + 2) % 3 );
        sortSpherePoints();
    }

    /**
     * @brief Rotates the points by 180 degrees around given axis.
     */
    void rotateBy180Around( size_t axis )
    {
        negateAxis( (axis + 1) % 3 );
        negateAxis( (axis + 2) % 3 );
        sortSpherePoints();
    }

    /**
     * @brief Permutate axes (X -> Y -> Z -> X).
     */
    void permutateAxes()
    {
        swapAxes( 1, 2 );
        swapAxes( 0, 1 );
        sortSpherePoints();
    }

    /**
     * @brief PCA transformation from original points
     * into the points defining the resulting cloud.
     */
    arma::mat transformation() const
    {
        return _coeff.t(); 
    }

    Vector eigenValues() const
    {
        return _eigenValues;
    }

    /**
     * @brief Cloud points as vector< Vector >.
     */
    std::vector< Vector > toVectors() const
    {
        std::vector< Vector > vecs;

        for ( const auto& [ radius, spherePoints ] : _spheres )
            for ( const Point& pt : spherePoints )
                vecs.push_back( Vector( { 
                    pt[0] * ERROR_MARGIN, 
                    pt[1] * ERROR_MARGIN, 
                    pt[2] * ERROR_MARGIN, 
                    1 } ) );

        return vecs;
    }

    bool operator==( const Cloud& o ) const
    {
        return _spheres == o._spheres;
    }

    bool operator>( const Cloud& o ) const
    {
        return _spheres > o._spheres;
    }

    bool operator<( const Cloud& o ) const
    {
        return _spheres < o._spheres;
    }

    size_t size() const
    {
        return _spheres.size();
    }

    auto begin() const
    {
        return _spheres.begin();
    }

    auto begin()
    {
        return _spheres.begin();
    }

    auto end() const
    {
        return _spheres.end();
    }

    auto end()
    {
        return _spheres.end();
    }

    void print() const
    {
        for ( const auto& [ radius, spherePoints ] : _spheres )
        {
            std::cout << "radius " << radius << ":\n";
            for ( const Point& pt : spherePoints ) 
                std::cout << pt[0] << "   " << pt[1] << "   " << pt[2] << "\n";
        }
    }

private:

    /**
     * @brief Normalize given points (data) to use their PCA coordinate system
     * (or its reflection in case the PCA transformation is a reflection,
     * so the shape given by the points does not change).
     * Returns normalized data and its transformation matrix.
     */
    std::tuple< arma::mat, arma::mat, arma::mat > normalize( const arma::mat& data ) const
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

        return std::tie( score, coeff, latent );
    }

    std::vector< std::pair< size_t, std::vector< Point > > > dataToSpherePoints( const arma::mat& data ) const
    {
        std::vector< std::pair< size_t, std::vector< Point > > > spheres;
        std::unordered_map< size_t, std::vector< Point > > spherePointsMap;

        for ( size_t pt = 0; pt < data.n_rows; ++pt )
        {
            Point roundedPoint;
            for ( size_t col = 0; col < 3; ++col )
                roundedPoint[col] = static_cast<int>( round( data( pt, col ) / ERROR_MARGIN ) );
            // Radius of sphere the point is on = point distance from origin
            // (after normalization, centroid is in origin)
            size_t radius = static_cast<size_t>( 
                round( rofi::configuration::matrices::distance( 
                    Vector( { data( pt, 0 ), data( pt, 1 ), data( pt, 2 ), 1 } ), 
                    Vector( { 0, 0, 0, 1 } ) ) 
                    / ERROR_MARGIN ) );
            spherePointsMap[radius].push_back( roundedPoint );
        }

        for ( auto& [ radius, spherePoints ] : spherePointsMap )
            spheres.push_back( std::make_pair( radius, spherePoints ) );
    
        return spheres;
    }

    void sortSpheres()
    {
        std::ranges::sort( _spheres );
    }

    void sortSpherePoints()
    {
        for ( auto& [ radius, spherePoints ] : _spheres )
            std::ranges::sort( spherePoints );
    }

    void swapAxes( size_t ax1, size_t ax2 )
    {
        for ( auto& [ radius, spherePoints ] : _spheres )
            for ( Point& pt : spherePoints )
                std::swap( pt[ax1], pt[ax2] );
    }

    void negateAxis( size_t ax )
    {
        for ( auto& [ radius, spherePoints ] : _spheres )
            for ( Point& pt : spherePoints )
                pt[ax] = -pt[ax];
    }
};

} // namespace rofi::shapereconfig
