/* Some of the math here is a duplicate of what is already in Matrix.h,
 * but I already had it defined for my specific purposes and didn't feel
 * comfortable touching what's already there */
#include "../configuration/Configuration.h"
#include "../configuration/IO.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

constexpr int size = 4;

double to_rad( double deg ){
    return ( deg * M_PI ) / 180;
}
double to_deg( double rad ){
    return ( 180 * rad ) / M_PI;
}
double vec_dist( const std::vector< double >& a, const std::vector< double >& b ){
    double dist = 0;
    for( size_t i = 0; i < 3; ++i ){
        dist += std::pow( a[ i ] - b[ i ], 2 ) ;
    }
    return std::sqrt( dist );
}

double rotx( const std::vector< double >& a, const std::vector< double >& b ){
    return atan2( a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ], a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ]  );
}
double rotz( const std::vector< double >& a, const std::vector< double >& b ){
    return atan2( a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ], a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] );
}

double eq( const double a, const double b ){
    return a - b < 0.001;
}

double mod( const double a, const double b ){
    double res = a;
    while( res > b ){
        res -= b;
    }
    return res;
}
std::vector< double > operator+( const std::vector< double >& a, const std::vector< double >& b ){
    std::vector< double > res;
    res.push_back( a[ 0 ] + b[ 0 ] );
    res.push_back( a[ 1 ] + b[ 1 ] );
    res.push_back( a[ 2 ] + b[ 2 ] );
    res.push_back( 1.0 );
    return res;
}

std::vector< double > operator-( const std::vector< double >& a, const std::vector< double >& b ){
    std::vector< double > res;
    res.push_back( a[ 0 ] - b[ 0 ] );
    res.push_back( a[ 1 ] - b[ 1 ] );
    res.push_back( a[ 2 ] - b[ 2 ] );
    res.push_back( 1.0 );
    return res;
}

std::vector< double > operator*( const std::vector< double >& a, double b ){
    std::vector< double > res;
    for( double num : a ){
        res.push_back( num * b );
    }
    return res;
}

// debug purposes
std::ostream& operator<<( std::ostream& os, const std::vector< double >& a ){
    assert( a.size() >= 3 );
    os << a[ 0 ] << " " << a[ 1 ] << " " << a[ 2 ] << '\n';
    return os;
}

struct rot_matrix {
    std::vector< std::vector< double > > m;
    rot_matrix( double alpha, double beta, double gamma ){
        m = {
                { std::cos( alpha ) * std::cos( beta ),
                  std::cos( alpha ) * std::sin( beta ) * std::sin( gamma ) - std::sin( alpha ) * std::cos( gamma ),
                  std::cos( alpha ) * std::sin( beta ) * std::cos( gamma ) + std::sin( alpha ) * std::sin( gamma ) },
                { std::sin( alpha ) * std::cos( beta ),
                  std::sin( alpha ) * std::sin( beta ) * std::sin( gamma ) + std::cos( alpha ) * std::cos( gamma ),
                  std::sin( alpha ) * std::sin( beta ) * std::cos( gamma ) - std::cos( alpha ) * std::sin( gamma ) },
                { -std::sin( beta ), std::cos( beta ) * std::sin( gamma ), std::cos( beta ) * std::cos( gamma ) }
            };
    }
    friend std::vector< double > operator*( const rot_matrix& a, const std::vector< double >& vec ){
        std::vector< double > res;
        double current = 0;
        for( int i = 0; i < 3; ++i ){
            for( int j = 0; j < 3; ++j ){
                current += a.m[ i ][ j ] * vec[ j ];
            }
            res.push_back( current );
            current = 0;
        }
        res.push_back( 1.0 );
        return res;
    }
};

struct t_matrix {
    std::vector< std::vector< double > > m;

    t_matrix() : m( size, std::vector( size, 0.0 ) ) {}

    t_matrix( double rx, double tx, double tz, double rz ) {
        m = {
                { std::cos( rz ), -std::sin( rz ), 0, tx },
                { std::sin( rz ) * std::cos( rx ), std::cos( rz ) * std::cos( rx ), -std::sin( rx ), -tz * std::sin( rx ) },
                { std::sin( rz ) * std::sin( rx ), std::cos( rz ) * std::sin( rx ), std::cos( rx ), tz * std::cos( rx ) },
                { 0, 0, 0, 1 }
            };
    }

    friend t_matrix operator*( const t_matrix& a, const t_matrix& b ){
        t_matrix c;
        double current = 0;
        for( int i = 0; i < size; ++i ){
            for( int j = 0; j < size; ++j ){
                for( int k = 0; k < size; ++k ){
                    current += a.m[ i ][ k ] * b.m[ k ][ j ];
                }
                c.m[ i ][ j ] = current;
                current = 0;
            }
        }
        return c;
    }

    friend std::vector< double > operator*( const t_matrix& a, const std::vector< double >& vec ){
        std::vector< double > res;
        double current = 0;
        for( int i = 0; i < size; ++i ){
            for( int j = 0; j < size; ++j ){
                current += a.m[ i ][ j ] * vec[ j ];
            }
            res.push_back( current );
            current = 0;
        }
        return res;
    }

    double simplify( std::vector< double >& vec, size_t leading_pos ) const {
        if( vec.size() == 0 )
            return { 1 };
        double divide_by = vec[leading_pos];
        for( size_t pos = leading_pos ; pos < vec.size(); pos++ ){
            vec[ pos ] = vec[ pos ] / divide_by;
        }
        return divide_by;
    }

    // t_matrix inv() const {
    //     t_matrix inverse;
    //     t_matrix mat = *this;
    //     for (size_t i = 0; i < size; i++)
    //         inverse.m[ i ][ i ] = 1;
    //     size_t leading_pos = 0;
    //     size_t row = 0;
    //     size_t to_swap = 0;
    //     for ( ; leading_pos < size; leading_pos++){
    //         to_swap = row + 1;
    //         while( to_swap < size && mat.m[ row ][ leading_pos ] == 0 ){
    //             if( mat.m[ to_swap][ leading_pos ] != 0 ){
    //                         std::iter_swap( mat.m.begin() + row, mat.m.begin() + to_swap );
    //                         std::iter_swap( inverse.m.begin() + row, inverse.m.begin() + to_swap );
    //                 }
    //                 to_swap++;
    //         }
    //         if( to_swap == size && mat.m[ row ][ leading_pos ] == 0 ){
    //             continue;
    //         }
    //         inverse.m[ row ] = inverse.m[ row ] * ( 1.0 / simplify( mat.m[ row ], leading_pos ) );
    //         for (size_t lower = 0; lower < size; lower++){
    //             if( mat.m[ lower ][ leading_pos ] == 0 || lower == row)
    //                 continue;
    //             double multiple = mat.m[ lower ][ leading_pos ];
    //             mat.m[ lower ] = mat.m[ lower ] - mat.m[ row ] * multiple;
    //             inverse.m[ lower ] = inverse.m[ lower ] - inverse.m[ row ] * multiple;
    //         }
    //         if( ++row == size )
    //             break;

    //     }
    //     return inverse;
    // }
    t_matrix inv() const {
        t_matrix inverse;
        t_matrix rotation;
        t_matrix translation;

        for( int i = 0; i < 4; ++i ){
            translation.m[ i ][ i ] = 1.0;
            rotation.m[ i ][ i ] = 1.0;
        }
        for( int i = 0; i < 3; ++i ){
            for( int j = 0; j < 3; j++ ){
                rotation.m[ i ][ j ] = m[ j ][ i ];
            }
        }
        for( int i = 0; i < 3; ++i ){
            translation.m[ i ][ 3 ] = -m[ i ][ 3 ];
        }
        inverse = rotation * translation;
        return inverse;
    }

};

struct joint {
    double rx, tx, tz, rz;

    joint() = default;
    joint( double rx, double tx, double tz, double rz ) : rx( rx ), tx( tx ), tz( tz ), rz( rz ) { }

    t_matrix to_previous(){
        return t_matrix( rx, tx, tz, rz );
    }
    t_matrix from_previous(){
        return t_matrix( rx, tx, tz, rz ).inv();
    }
};
double constexpr error = 0.02;

struct kinematic_chain {

    Configuration config;

    std::vector< int > ids; // fixed order for modules based on how they're connected;

    std::vector< joint > chain; // chain of kinematic joints

    std::vector< std::vector< double > > joint_positions;

    std::vector< std::vector< double > > goals;

    // Initialize the kinematic chain and positions
    void init( std::vector< double > starting_pos = { 0.0, 0.0, 0.0, 0.0 }){
        // TODO: More sophisticated way to choose the fix point, this could cause problems
        ids = config.getIDs();
        std::sort( ids.begin(), ids.end() );

        chain.emplace_back( starting_pos[ 0 ],
                            starting_pos[ 1 ],
                            starting_pos[ 2 ],
                            starting_pos[ 3 ] );

        chain.emplace_back( to_rad( config.getModule( ids[ 0 ] ).getJoint( Joint::Alpha ) ),
                            0,
                            1,
                            to_rad( config.getModule( ids[ 0 ] ).getJoint( Joint::Gamma ) ) );

        for( size_t i = 1; i < ids.size(); ++i ){
            chain.emplace_back( to_rad( config.getModule( ids[ i - 1 ] ).getJoint( Joint::Beta ) ),
                                0,
                                1,
                                0 );
            chain.emplace_back( to_rad( config.getModule( ids[ i ] ).getJoint( Joint::Alpha ) ),
                                0,
                                1,
                                to_rad( config.getModule( ids[ i ] ).getJoint( Joint::Gamma ) ) );
        }
        // an imaginary joint at the end, helps us get the rotation/connection of two modules right
        chain.emplace_back( 0, 0, 1, 0 );

        for( const auto& j : chain ){
            joint_positions.push_back( { 0.0, 0.0, 0.0, 1.0 } );
        }

        set_position( 0 );
    }

    // Set the position of joints following the given joint
    void set_position( size_t joint_index ){
        for( size_t j = joint_index; j < chain.size(); ++j ){
            size_t k = j + 1;
            auto pos = std::vector{ 0.0, 0.0, 0.0, 1.0 };
            while( k-- ){
                pos = chain[ k ].to_previous() * pos;
            }
            joint_positions[ j ] = pos;
        }
    }

    // Move chain in such a way that given joint reaches target position
    bool reach( size_t joint_index, size_t goal_index, bool fix_next = false ){

        std::vector< double > local_end = joint_positions[ joint_index ];
        std::vector< double > local_goal = goals[ goal_index ];

        while( vec_dist( joint_positions[ joint_index ], goals[ goal_index ] ) > error ){

            for( size_t i = joint_index; i != 0; --i ){
                local_end = joint_positions[ joint_index ];
                local_goal = goals[ goal_index ];
                for( int j = 0; j <= i; ++j ){
                    local_end = chain[ j ].from_previous() * local_end;
                    local_goal = chain[ j ].from_previous() * local_goal;
                }

                double current_x = chain[ i ].rx;
                double rotation_x = rotx( local_end, local_goal );

                chain[ i ].rx = std::clamp( current_x + rotation_x, -M_PI / 2, M_PI / 2 );

                double current_z = chain[ i ].rz;
                double rotation_z = 0.0;
                if( i % 2 == 1 ){
                    rotation_z = rotz( local_end, local_goal );
                    chain[ i ].rz = mod( current_z + rotation_z, 360.0 );
                }

                set_position( i );

                if( fix_next ){
                    while( vec_dist( joint_positions[ joint_index ], goals[ goal_index + 1 ] ) > 1.0 + error ||
                           vec_dist( joint_positions[ joint_index - 1 ], goals[ goal_index + 1 ] ) < 1.0){
                        rotation_x /= 2.0;
                        rotation_z /= 2.0;
                        chain[ i ].rx = std::clamp( current_x + rotation_x, -M_PI / 2, M_PI / 2 );
                        chain[ i ].rz = mod( current_z + rotation_z, 360.0 );
                        set_position( i );
                    }
                }

            }
 
        }

        return true;
    }
};

