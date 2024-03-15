#pragma once

#include <configuration/Matrix.h>

/*
  Standard NURBS implementation.
  Based on https://en.wikipedia.org/wiki/Non-uniform_rational_B-spline.

  We use points in homogenous coordinates, where the first 3 parameters
  define standard position in cartesian space, and the 4th represents the
  weight of a given control points.

  Given a sequence of control points and knots, we get specific points on
  the curve. Correctness of the input parameters needs to be checked externally.
 */

namespace rofi::geometry {

using namespace rofi::configuration::matrices;

/*
  Parameters are named with single letters to be consistent with the origin
  of the equations
 */

struct Curve {

    std::vector< Vector > control_points;

    std::vector< double > knots;

    int order = 2;

    double N( size_t i, int n, double u ){
        if( n == 0 ){
            return u >= knots[ i ] && u < knots[ i + 1 ] ? 1.0 : 0;
        }
        auto f = [&]( size_t fi ){
            if( knots[ fi + n ] - knots[ fi ] == 0.0 ){
                return 0.0;
            }
            return ( u - knots[ fi ] ) / ( knots[ fi + n ] - knots[ fi ] );
        };
        auto g = [&]( size_t gi ){
            return 1 - f( gi );
        };
        return f( i ) * N( i, n - 1, u ) + g( i + 1 ) * N( i + 1, n - 1, u );
    }

    Vector get_point( double u ){
        Vector result = { 0.0, 0.0, 0.0 };
        for( size_t i = 0; i < control_points.size(); i++ ){
            double div = 0.0;
            for( size_t j = 0; j < control_points.size(); j++ ){
                div += N( j, order, u ) * control_points[ j ][ 3 ];
            }

            result = ( ( N( i, order, u ) * control_points[ i ][ 3 ] ) / div ) * control_points[ i ] + result;
        }
        return result;
    }

};

} // namespace rofi::geometry
