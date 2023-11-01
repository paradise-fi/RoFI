#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include <configuration/Matrix.h>

/*
   This file implements general purpose shapes.
   Shapes are defined in a standard way, and have their associated operations
   such as equality, maximal value in a given direction and a bounding box,
   whenever such an operation makes sense. In order to work with shapes within
   an AABB, bounding boxes and collision checking are defined for the different
   shapes.
 */

namespace rofi::geometry {

using namespace rofi::configuration::matrices;

constexpr double epsilon = 1.0e-10;

/* Utility operations */

/* dot product */
inline double operator*( const Vector& a, const Vector& b ){
    return a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ];
}

inline double magnitude( const Vector& vector ){
    return std::sqrt( std::pow( vector[ 0 ], 2 ) +
                      std::pow( vector[ 1 ], 2 ) +
                      std::pow( vector[ 2 ], 2 ) );
}

/*
   Boxes are represented with their center and x, y, z dimensions.
   Varying rotations are not considered for our purpsoes.
 */
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

struct Line {
    Vector origin;
    Vector direction;

    Line( Vector point, Vector direction ) : origin( point ), direction( direction ) {}

    friend bool operator==( const Line& a, const Line& b ){
        return equals( a.origin, b.origin ) && equals( a.direction, b.direction );
    }
    friend bool operator!=( const Line&, const Line& ) = default;
};

struct Segment {
    Vector origin;
    Vector end;

    Segment( Vector origin, Vector end ) : origin( origin ), end( end ) {}

    Box bounding_box() const {
        Vector diagonal = end - origin;
        return Box( origin + diagonal / 2,
                    std::fabs( diagonal[ 0 ] ),
                    std::fabs( diagonal[ 1 ] ),
                    std::fabs( diagonal[ 2 ] ) );
    }

    double max( size_t axis ) const {
        return std::max( origin[ axis ], end[ axis ] );
    }
    double min( size_t axis ) const {
        return std::min( origin[ axis ], end[ axis ] );
    }

    friend bool operator==( const Segment& a, const Segment& b ){
        return equals( a.origin, b.origin ) && equals( a.end, b.end );
    }
    friend bool operator!=( const Segment&, const Segment& ) = default;
};

struct Plane {
    Vector origin;
    Vector normal;
    double width; // when collision checking for objects moving along a plane, we want to widen it to match the moving object

    Plane( Vector origin, Vector normal, double width = 0 ) : origin( origin ), normal( normal ), width( width ){}
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
inline bool collide( const Vector& point, const Box& box ){
    return collide( box, point );
}

// box collides with ball when a box widened by the ball radius colides with the center
inline bool collide( const Box& box, const Sphere& ball ){
    Box widened = box;
    for( size_t i = 0; i < 3; i++ ){
        widened.dimensions[ i ] += ball.radius * 2;
    }
    return collide( widened, ball.center );
}

inline bool collide( const Sphere& ball, const Box& box ){
    return collide( box, ball );
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

// returns ( NaN, NaN ) if the objects don't collide
inline std::pair< double, double > intersection_distance( const Line& line, const Sphere& sphere ){
    double length = std::pow( magnitude( line.direction ), 2 );
    double root = std::sqrt( std::pow( line.direction * ( line.origin - sphere.center ), 2.0 )
        - length * ( std::pow( magnitude( line.origin - sphere.center ), 2.0 ) - std::pow( sphere.radius, 2 ) ) );
    double tmp = -( line.direction * ( line.origin - sphere.center ) );

    return { ( tmp + root ) / length, ( tmp - root ) / length };
}

inline bool collide( const Line& line, const Sphere& sphere ){
    return !std::isnan( intersection_distance( line, sphere ).first );
}
inline bool collide( const Sphere& sphere, const Line& line ){
    return collide( line, sphere );
}

inline bool collide( const Segment& segment, Sphere sphere ){
    auto [ a, b ] = intersection_distance( Line( segment.origin, segment.end - segment.origin ), sphere );
    return ( a >= 0.0 && a <= 1.0 ) || ( b >= 0.0 && b <= 1.0 );
}

inline bool collide( const Sphere& sphere, const Segment& segment ){
    return collide( segment, sphere );
}

// Raycasting equation, only works in one direction
inline std::pair< double, double > intersection_distance( const Line& line, const Box& box ){
    constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    constexpr double inf = std::numeric_limits< double >::infinity();
    double t_near = -inf;
    double t_far = inf;
    for( size_t axis : { 0, 1, 2 } ){
        if( line.direction[ axis ] == 0.0 &&
            ( line.origin[ axis ] > box.max( axis ) || line.origin[ axis ] < box.min( axis ) ) )
        {
            return { nan, nan };
        }
        double t1 = ( box.min( axis ) - line.origin[ axis ] ) / line.direction[ axis ];
        double t2 = ( box.max( axis ) - line.origin[ axis ] ) / line.direction[ axis ];
        if( t1 > t2 ){
            std::swap( t1, t2 );
        }
        if( t1 > t_near ){
            t_near = t1;
        }
        if( t2 < t_far ){
            t_far = t2;
        }
        if( t_near > t_far ){
            return { nan, nan };
        }
        if( t_far < 0 ){
            return { nan, nan };
        }
    }
    return { t_near, t_far };
}

inline bool collide( const Line& line, const Box& box ){
    return !std::isnan( intersection_distance( line, box ).first )
        || !std::isnan( intersection_distance( Line( line.origin, -line.direction ), box ).first );
}
inline bool collide( const Box& box, const Line& line ){
    return collide( line, box );
}

inline bool collide( const Segment& segment, Box box ){
    if( collide( segment.origin, box ) || collide( segment.end, box ) ){
        return true;
    }
    auto [ a, b ] = intersection_distance( Line( segment.origin, segment.end - segment.origin ), box );
    return ( a >= 0.0 && a <= 1.0 ) || ( b >= 0.0 && b <= 1.0 );
}
inline bool collide( const Box& box, const Segment& segment ){
    return collide( segment, box );
}

inline bool collide( const Plane& plane, const Sphere& ball ){
    double dist = ( ball.center - plane.origin ) * plane.normal;
    return dist + plane.width < ball.radius;
}

inline bool collide( const Sphere& ball, const Plane& plane ){
    return collide( plane, ball );
}

inline bool collide( const Plane& plane, const Box& box ){
    Vector extent = box.dimensions / 2; // Compute positive extents

    // Compute the projection interval radius of b onto L(t) = b.c + t * p.n
    double radius = extent[0] * std::fabs( plane.normal[ 0 ] ) + extent[1] * std::fabs( plane.normal[ 1 ] ) + extent[ 2 ] * std::fabs( plane.normal[ 2 ] );

    // Compute distance of box center from plane
    double s = plane.normal * box.center - plane.normal * plane.origin;

    // Intersection occurs when distance s falls within [-r,+r] interval
    return std::fabs( s ) <= radius + plane.width;
}

inline bool collide( const Box& box, const Plane& plane ){
    return collide( plane, box );
}

} // namespace rofi::geometry
