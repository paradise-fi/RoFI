#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include <configuration/Matrix.h>

namespace rofi::geometry {

using namespace rofi::configuration::matrices;

constexpr double epsilon = 1.0e-10;

struct Box {
    Vector center;
    Vector dimensions;
    Box( Vector center, double x = 1.0, double y = 1.0, double z = 1.0 ) :
        center( center ), dimensions( { x, y, z, 0 } ) {}

    Box bounding_box() const {
        return *this;
    }

    double max( size_t axis ) const {
        return center[ axis ] + dimensions[ axis ] / 2;
    }
    double min( size_t axis ) const {
        return center[ axis ] - dimensions[ axis ] / 2;
    }

    double volume() const {
        return dimensions[ 0 ] + dimensions[ 1 ] + dimensions[ 2 ];
    }

    friend bool operator==( const Box& a, const Box& b ){
        return equals( a.center, b.center ) && equals( a.dimensions, b.dimensions );
    }
    friend bool operator!=( const Box&, const Box& ) = default;

    friend std::ostream& operator<<( std::ostream& os, const Box& b ){
        os << "Center: " << b.center[ 0 ] << " " << b.center[ 1 ] << " " << b.center[ 2 ]
           << "\nDimensions: " << b.dimensions[ 0 ] << " " << b.dimensions[ 1 ] << " " << b.dimensions[ 2 ] << '\n';
        return os;
    }
};

struct Sphere {
    Vector center;
    double radius;

    /* Default radius is <0.5 to make sure spheres with distance 1 don't collide
       due to rounding errors */
    Sphere( Vector center, double radius = 0.5 - epsilon ) : center( center ), radius( radius ) {}

    Box bounding_box() const {
        return Box( center, radius * 2, radius * 2, radius * 2 );
    }

    double max( size_t axis ) const {
        return center[ axis ] + radius;
    }
    double min( size_t axis ) const {
        return center[ axis ] - radius;
    }

    friend bool operator==( const Sphere& a, const Sphere& b ){
        return equals( a.center, b.center ) && std::fabs( a.radius - b.radius ) < 0.001;
    }
    friend bool operator!=( const Sphere&, const Sphere& ) = default;
};

template< typename T, typename S >
Box bounding_box( const T& a, const S& b ){
    Box bound( Vector{ 0, 0, 0, 1 } );
    Vector min = { 0, 0, 0, 1 };
    Vector max = { 0, 0, 0, 1 };

    for( size_t axis : { 0, 1, 2 } ){
        min[ axis ] = std::min( a.min( axis ),
                                b.min( axis ) );
        max[ axis ] = std::max( a.max( axis ),
                                b.max( axis ) );
    }
    bound.center = ( min + max ) / 2;
    bound.dimensions = max - min;
    return bound;
}

inline double magnitude( const Vector& vector ){
    return std::sqrt( std::pow( vector[ 0 ], 2 ) +
                      std::pow( vector[ 1 ], 2 ) +
                      std::pow( vector[ 2 ], 2 ) );
}

inline bool collide( const Sphere& ball, const Vector& point ){
    double dist = magnitude( ball.center - point );
    return dist < ball.radius;
}

inline bool collide( const Sphere& ball, const Sphere& ball2 ){
    double dist = magnitude( ball.center - ball2.center );
    return dist < ball.radius + ball2.radius;
}

// point is inside of box when distance on all of the 3 axes is smaller or equal to its size
inline bool collide( const Box& box, const Vector& point ){
    for( size_t i = 0; i < 3; i++ ){
        if( std::fabs( box.center[ i ] - point[ i ] ) >= box.dimensions[ i ] / 2.0 ){
            return false;
        }
    }
    return true;
}

// box collides with ball when a box widened by the ball radius colides with the center
inline bool collide( const Box& box, const Sphere& ball ){
    Box widened = box;
    for( size_t i = 0; i < 3; i++ ){
        widened.dimensions[ i ] += ball.radius * 2;
    }
    return collide( widened, ball.center );
}

// boxes collide when the distance of centers is smaller than their combined size on any axis
inline bool collide( const Box& box, const Box& box2 ){
    for( size_t i = 0; i < 3; i++ ){
        if( std::fabs( box.center[ i ] - box2.center[ i ] )
            < box.dimensions[ i ] / 2 + box2.dimensions[ i ] / 2 )
        {
            return true;
        }
    }
    return false;
}

} // namespace rofi::geometry
