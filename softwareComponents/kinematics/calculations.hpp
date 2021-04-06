#include "Configuration.h"
#include "IO.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

inline double to_rad( double deg ){
    return ( deg * M_PI ) / 180;
}
inline double to_deg( double rad ){
    return ( 180 * rad ) / M_PI;
}

inline double angle( const Vector& a, const Vector& b ){
    return std::acos( ( a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ] ) /
                      ( std::sqrt( std::pow( a[ 0 ], 2 ) + std::pow( a[ 1 ], 2 ) + std::pow( a[ 2 ], 2 ) ) *
                        std::sqrt( std::pow( b[ 0 ], 2 ) + std::pow( b[ 1 ], 2 ) + std::pow( b[ 2 ], 2 ) ) ) );
}

inline double rotx( const Vector& a, const Vector& b ){
    return std::atan2( a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ], a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ]  );
}
inline double roty( const Vector& a, const Vector& b ){
    return std::atan2( a[ 0 ] * b[ 2 ] - a[ 2 ] * b[ 0 ], a[ 0 ] * b[ 0 ] + a[ 2 ] * b[ 2 ]  );
}
inline double rotz( const Vector& a, const Vector& b ){
    return std::atan2( a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ], a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] );
}

inline double eq( const double a, const double b ){
    return a - b < 0.001;
}

inline double mod( const double a, const double b ){
    double res = a;
    while( res > b ){
        res -= b;
    }
    return res;
}

inline Vector operator*( double num, const Vector& vec ){
    return Vector({ vec[ 0 ] * num,
                    vec[ 1 ] * num,
                    vec[ 2 ] * num,
                    vec[ 3 ] });
}
inline Vector operator+( const Vector& a, const Vector& b ){
    return Vector({ a[ 0 ] + b[ 0 ],
                    a[ 1 ] + b[ 1 ],
                    a[ 2 ] + b[ 2 ],
                    1.0 });
}

inline Vector operator-( const Vector& a, const Vector& b ){
    return Vector({ a[ 0 ] - b[ 0 ],
                    a[ 1 ] - b[ 1 ],
                    a[ 2 ] - b[ 2 ],
                    1.0 });
}

inline Vector operator/( const Vector& vec, double num ){
    return Vector({ vec[ 0 ] / num,
                    vec[ 1 ] / num,
                    vec[ 2 ] / num,
                    vec[ 3 ] });
}

inline Vector cross_product( const Vector& a, const Vector& b ){
    return Vector({ a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ],
                    a[ 2 ] * b[ 0 ] - a[ 0 ] * b[ 2 ],
                    a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ] } );
}

inline double magnitude( const Vector& vector ){
    return std::sqrt( std::pow( vector[ 0 ], 2 ) +
                      std::pow( vector[ 1 ], 2 ) +
                      std::pow( vector[ 2 ], 2 ) );
}

inline double arbitrary_magnitude( const arma::vec& vector ){
    double sum = 0;
    for( double d : vector ){
        sum += std::pow( d, 2 );
    }
    return std::sqrt( sum );
}
/* dot product */
inline double operator*( const Vector& a, const Vector& b ){
    return a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ];
}

/* To invert the rotation, you transpose it. To invert a translation,
 * negate it. This matrix is the result of both */
inline Matrix inverse( Matrix m ){
    Matrix translate = identity;
    Matrix rotation = identity;
    for( int i = 0; i < 3; ++i ){
        for( int j = 0; j < 3; ++j ){
            rotation.at( i, j ) = m.at( j, i );
        }
    }
    for( int i = 0; i < 3; ++i ){
        translate.at( i, 3 ) = -m.at( i, 3 );
    }
    return rotation * translate;
}

