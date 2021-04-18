#include "kinematics.hpp"

kinematic_rofibot::kinematic_rofibot( Configuration new_config, bool fixed )
                    : config( new_config ), fixed( fixed )
{

    std::vector< int > ids = config.getIDs();
    std::vector< bool > taken;
    std::sort( ids.begin(), ids.end() );
    
    if( fixed ){
        if( !config.isValid() )
            throw std::exception();
        arms.emplace_back( ids.begin(), ids.end() );
    } else {
        for( int id : ids ){
            auto edges = config.getEdges( id );
            int current = id;
            std::unordered_set< int > exclude;
            if( edges.size() == 1 ){
                arms.emplace_back( chain{ id } );
                auto edge = edges.front();
                while( ( edges = config.getEdges( current, exclude ) ).size() == 1 ){
                    exclude = { current };
                    current = edges.front().id2();
                    arms.back().push_front( current );
                }
            }
        }
    }
    
    config.computeMatrices();

};

kinematic_rofibot::kinematic_rofibot( std::string file_name, bool fixed )
{
    std::ifstream input( file_name );
    if( !input.is_open() ){
        std::cerr << "Couldn't open file \'" << file_name << "\'" << std::endl;
        return;
    }

    Configuration new_config;

    IO::readConfiguration( input, new_config );

    *this = kinematic_rofibot( new_config, fixed );
}

bool kinematic_rofibot::connect_ccd( int a, int b, int max_iterations )
{
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector goal_a = pointing_to( arms[ a ] );
    Vector goal_b = pointing_to( arms[ b ] );

    int iterations = 0;
    while( distance( goal_a, position_b ) > 0.01 || distance( goal_b, position_a ) > 0.01 ){
        ccd2( position_b, { 0.0, 0.0, 0.0 }, arms[ a ], 1 );
        position_a = end_effector( arms[ a ] );
        goal_a = pointing_to( arms[ a ] );
    
        ccd2( position_a, { 0.0, 0.0, 0.0 }, arms[ b ], 1 );
        position_b = end_effector( arms[ b ] );
        goal_b = pointing_to( arms[ b ] );
    
        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }

    return true;
}

bool kinematic_rofibot::connect_fabrik( int a, int b, int max_iterations ){
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector z_a = pointing_to( arms[ a ] );
    Vector z_b = pointing_to( arms[ b ] );

    Vector y_a = get_global( arms[ a ].back(), 1, { 0, 1, -1, 1 } );
    Vector y_b = get_global( arms[ b ].back(), 1, { 0, 1, -1, 1 } );
 
    int iterations = 0;
    while( distance( position_a, z_b ) > 0.01 || distance( position_b, z_a ) > 0.01 ){

        fabrik( z_a, y_a - z_a, position_a - z_a, arms[ b ] );

        position_b = end_effector( arms[ b ] );
        z_b = pointing_to( arms[ b ] );
        y_b = get_global( arms[ b ].back(), 1, { 0, 1, -1, 1 } );

        fabrik( z_b, y_b - z_b, position_b - z_b, arms[ a ] );

        std::cout << IO::toString( config );

        position_a = end_effector( arms[ a ] );
        z_a = pointing_to( arms[ a ] );
        y_a = get_global( arms[ a ].back(), 1, { 0, 1, -1, 1 } );

        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }
    return true;
}

bool kinematic_rofibot::ccd( const Vector& goal, const Vector& x_frame, const Vector& y_frame,
                             const Vector& z_frame, const chain& arm, int max_iterations )
{
    double wp = 1.0, wa = 1.0;
    double k1, k2, k3;

    double rotation;

    Vector end_position = end_effector( arm );
    int iterations = 0;
    while( distance( end_position, goal ) > error ||
           distance( pointing_to( arm ), goal + z_frame ) > error ){
        for( auto it = arm.rbegin(); it != arm.rend(); ++it ){
            for( Joint j : { Beta, Gamma, Alpha } ){

                Vector local_end = get_local( *it, j == Beta, end_position );
                Vector local_goal = get_local( *it, j == Beta, goal );

                Vector current_x = get_global( arm.back(), 1, { -1, 0, 0, 1 } ) - end_position;
                Vector current_y = get_global( arm.back(), 1, { 0, 1, 0, 1 } ) - end_position;
                Vector current_z = get_global( arm.back(), 1, { 0, 0, -1, 1 } ) - end_position;

                wp = 1 + ( std::min( magnitude( local_end ), magnitude( local_goal ) ) /
                            std::max( magnitude( local_end ), magnitude( local_goal ) ) );

                Vector axis = j == Gamma ? Z : X;
                k1 = wp * ( local_goal * axis ) * ( local_end * axis ) +
                     wa * ( ( x_frame * axis ) * ( current_x * axis ) +
                            ( y_frame * axis ) * ( current_y * axis ) +
                            ( z_frame * axis ) * ( current_z * axis ) );

                k2 = wp * ( local_goal * local_end ) + wa * ( ( x_frame * current_x ) +
                                                              ( y_frame * current_y ) +
                                                              ( z_frame * current_z ) );

                k3 = axis * ( wp * cross_product( local_end, local_goal ) +
                              wa * ( cross_product( current_x, x_frame ) +
                                     cross_product( current_y, y_frame ) +
                                     cross_product( current_z, z_frame ) ) );

                double candidate = std::atan( k3 / ( k2 - k1 ) );

                auto second_derivative = [=]( double rot ) {
                    return ( k1 - k2 ) * std::cos( rot ) + k3 * ( -1 ) * std::sin( rot );
                };

                if( second_derivative( candidate ) < 0 ){
                    rotation = to_deg( candidate );
                } else if( candidate + M_PI < M_PI && second_derivative( candidate + M_PI ) < 0 ){
                    rotation = to_deg( candidate + M_PI );
                } else if( candidate - M_PI > -M_PI && second_derivative( candidate - M_PI ) < 0 ){
                    rotation = to_deg( candidate - M_PI );
                } else {
                    rotation = 0;
                }

                double current = config.getModule( *it ).getJoint( j );

                if( j != Gamma )
                    rotation = std::clamp( rotation + current, -90.0, 90.0 ) - current;

                rotate_if_valid( *it, j, rotation );
                end_position = end_effector( arm );
            }
        }
        if( ++iterations == max_iterations ){
            return false;
        }
    }
    return true;
}
// currently made to work for connection, might need to separate the two cases
bool kinematic_rofibot::ccd2( const Vector& goal, const std::vector< double >& rotation,
                              const chain& arm, int max_iterations )
{
    Vector end_goal =
        rotate( rotation[ 0 ], X ) *
        rotate( rotation[ 1 ], Y ) *
        rotate( rotation[ 2 ], Z ) *
        Vector( { 0, 0, 1, 1 } ) + goal;
    Vector end_position = pointing_to( arm );
    int iterations = 0;
    while( distance( end_position, goal ) > error ){
        for( auto it = arm.rbegin(); it != arm.rend(); ++it ){
            for( Joint j : { Beta, Gamma, Alpha } ){
                if( it == arm.rbegin() )
                    rotate_to( end_position, goal, *it, j, true, true );
                else
                    rotate_to( end_position, goal, *it, j );
                
                end_position = pointing_to( arm );
            }
        }
        if( ++iterations == max_iterations )
            return false;
    }
    return true;
}


bool kinematic_rofibot::rotate_to( const Vector& end_pos, const Vector& end_goal, int i, Joint joint, bool check_validity, bool round )
{
    Vector local_end;
    Vector local_goal;

    double current = config.getModule( i ).getJoint( joint );
    double rotation;

    switch( joint ){
        case Alpha:
            local_end = get_local( i, 0, end_pos );
            local_goal = get_local( i, 0, end_goal );
            rotation = to_deg( rotx( local_end, local_goal ) );
            rotation = std::clamp( current + rotation, -90.0, 90.0 ) - current;
            break;
        case Beta:
            local_end = get_local( i, 1, end_pos );
            local_goal = get_local( i, 1, end_goal );
            rotation = -to_deg( rotx( local_end, local_goal ) );
            rotation = std::clamp( current + rotation, -90.0, 90.0 ) - current;
            break;
        case Gamma:
            local_end = get_local( i, 0, end_pos );
            local_goal = get_local( i, 0, end_goal );
            rotation = to_deg( rotz( local_end, local_goal ) );
            break;
        default:
            throw std::exception();
    }

    if( round ){
        if( rotation + current < -45.0 ){
            rotation = -90;
        } else if( rotation + current < 45.0 ){
            rotation = 0;
        } else {
            rotation = 90;
        }
        rotation -= current;
    }

    if( !check_validity ){
        config.execute( Action( Action::Rotate( i, joint, rotation ) ) );
        return true;
    }

    return rotate_if_valid( i, joint, rotation );
}

bool kinematic_rofibot::rotate_if_valid( int module, Joint joint, double angle )
{
    config.execute( Action( Action::Rotate( module, joint, angle ) ) );
    if( !config.isValid() ){
        config.execute( Action( Action::Rotate( module, joint, -angle ) ) );
        config.computeMatrices();
        return false;
    }
    return true;
}

bool kinematic_rofibot::fabrik( const Vector& goal, const Vector& y_frame, const Vector& z_frame,
                                const chain& arm, int max_iterations )
{
    assert( rotation.size() == 3 );
    assert( max_iterations > 0 );

    std::vector< Vector > positions;
    for( int i : arm ){
        for( int j : { Alpha, Beta } ){
            positions.emplace_back( get_global( i, j ) );
        }
    }
    Vector base = positions[ 0 ];
    Vector second = positions[ 1 ];

    int iterations = 0;
    while( distance( end_effector( arm ), goal ) > 0.01 /*||
           distance( pointing_to( arm ), goal + z_frame ) > 0.01*/ ){
        
        /* save the last position and end the algorithm if it hasn't changed at all */
        Vector last_position = get_global( arm.back(), 1 );
        
        positions.back() = goal;

        // old style for loops for easier access to the next element

        /* Forward reaching */
        for( size_t i = positions.size() - 1; i--; ){
            /* The limit set on RoFIs: every other joint is projected on the
             * line between the next and previous one, because there is no 
             * rotation between two modules */
            if( i == positions.size() - 2 ){
                Vector normal = cross_product( z_frame, y_frame );
                positions[ i ] = positions[ i ] - ( normal * ( positions[ i ] - positions[ i + 1 ] ) ) * normal;
            } else if( i % 2 == 1 ){
                Vector direction = positions[ i + 2 ] - positions[ i + 1 ];
                Vector to_base = positions[ i - 1 ] - positions[ i + 1 ];
                Vector normal = cross_product( direction, to_base );
                positions[ i ] = positions[ i ] - ( normal * ( positions[ i ] - positions[ i + 1 ] ) ) * normal;
            }

            /* Sets the joint on the line between its previous position and
             * the next one, so that the link length is one **/
            double lambda = 1.0 / distance( positions[ i ], positions[ i + 1 ] );
            positions[ i ] = ( 1.0 - lambda ) * positions[ i + 1 ] + lambda * positions[ i ];

            /* If the new position is too close (doesn't respect the limit of rotation to 90°),
             * reposition it to the other side **/
            Vector prev = i == positions.size() - 2 ? goal + z_frame : positions[ i + 2 ];
            if( distance( prev, positions[ i ] ) < std::sqrt( 2.0 ) ){
                Vector normal = prev - positions[ i + 1 ];
                positions[ i ] = positions[ i ] - ( normal * ( positions[ i ] - positions[ i + 1 ] ) ) * normal;

                lambda = 1.0 / distance( positions[ i ], positions[ i + 1 ] );
                positions[ i ] = ( 1.0 - lambda ) * positions[ i + 1 ] + lambda * positions[ i ];
            }
        }

        positions[ 0 ] = base;
 
        /* Backward reaching, this time actually move the links */
        for( size_t i = 0; i < positions.size() - 1; ++i ){
            Joint current = i % 2 == 0 ? Alpha : Beta;
            
            Vector next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
            if( current == Beta ){
                rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], Gamma, true );
            }
            next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
            rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], current, true );

            positions[ i + 1 ] = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
        }

        rotate_to( pointing_to( arm ), end_effector( arm ) + z_frame, arm.back(), Beta );
        rotate_to( pointing_to( arm ), end_effector( arm ) + z_frame, arm.back(), Gamma );

        if( ++iterations == max_iterations || distance( last_position, get_global( arm.back(), 1 ) ) < 1e-10 ){
            return false;
        }
    }

    return true;
}

arma::mat33 get_rotation( const Matrix& matrix ){
    arma::mat33 result;
    for( int i = 0; i < 3; ++i ){
        for( int j = 0; j < 3; ++j ){
            result.at( i, j ) = matrix.at( i, j );
        }
    }
    return result;
}

arma::vec3 get_translation( const Matrix& matrix ){
    arma::vec3 result;
    for( int i = 0; i < 3; ++i ){
        result[ i ] = matrix.at( i, 3 );
    }
    return result;
}

arma::mat kinematic_rofibot::jacobian( const chain& arm ){
    int columns = arm.size() * 3;
    arma::mat result( 6, columns );

    arma::vec3 z1( { 0, 0, 1 } );
    arma::vec3 x1( { 1, 0, 0 } );

    for( int i = 0; i < columns; ++i ){
        arma::vec3 rotation;
        if( i % 3 == 0 )
            rotation = get_rotation( get_matrix( arm[ i / 3 ], 0 ) ) * x1;
        else if( i % 3 == 1 )
            rotation = get_rotation( get_matrix( arm[ i / 3 ], 0 ) ) * z1;
        else
            rotation = get_rotation( get_matrix( arm[ i / 3 ], 1 ) ) * x1;

        arma::vec3 current = arma::cross( rotation,
            get_translation( get_matrix( arm.back(), 1 ) ) - 
            get_translation( get_matrix( arm[ i / 3 ], i % 3 == 2 ) )
            );
        for( int j = 0; j < 3; ++j ){
            result( j, i ) = current[ j ];
        }
        
        for( int j = 0; j < 3; ++j ){
            result( j + 3, i ) = rotation[ j ];
        }
    }
    return result;
}

bool kinematic_rofibot::pseudoinverse( const Vector& goal, const Vector& x_frame, const Vector& z_frame,
                              const chain& arm, int max_iterations )
{
    config.computeMatrices();
    //Configuration old = config;

    Vector global_x = end_effector( arm ) + x_frame;
    Vector global_z = end_effector( arm ) + z_frame;

    double rz = rotz( get_local( arm.back(), 1, global_x ), { -1, 0, 0, 1 } );
    double rx = rotx( get_local( arm.back(), 1, global_z ), { 0, 0, -1, 1 } );
    double ry = roty( get_local( arm.back(), 1, global_x ), { -1, 0, 0, 1 } );

    int iterations = 0;
    while( distance( goal, end_effector( arm ) ) > 0.01 ||
            std::fabs( rz ) + std::fabs( rx ) + std::fabs( ry ) > 0.1 ){

        Vector pos = end_effector( arm );

        global_x = end_effector( arm ) + x_frame;
        global_z = end_effector( arm ) + z_frame;

        rz = rotz( get_local( arm.back(), 1, global_x ), { -1, 0, 0, 1 } );
        rx = rotx( get_local( arm.back(), 1, global_z ), { 0, 0, -1, 1 } );
        ry = roty( get_local( arm.back(), 1, global_x ), { -1, 0, 0, 1 } );
    
        arma::mat j = jacobian( arm );

        arma::vec diff = { goal[ 0 ] - pos[ 0 ], goal[ 1 ] - pos[ 1 ], goal[ 2 ] - pos[ 2 ], rx, ry, rz };
        arma::mat pseudo_inv = arma::trans( j ) * ( arma::inv( j * arma::trans( j ) ) );

        arma::mat I( 6, 6, arma::fill::eye );
        double error;
        /* Ad hoc constant; higher precision can reduce the number of iterations
         * but increase computation per iteration */
        double prec = 1e-10;
        do {
            error = arbitrary_magnitude( ( I - j * pseudo_inv ) * diff );
            if( error > prec ){
                diff /= 2.0;
            }
        } while( error > prec );

        /* Normalising the vector seems to work with singularities, might need
         * further testing */
        arma::vec velocities = /*arma::normalise*/( pseudo_inv * diff );

        for( int i = 0; i < velocities.size(); ++i ){
            Joint joint;
            if( i % 3 == 0 ){
                joint = Alpha;
            }
            if( i % 3 == 1 )
                joint = Gamma;
            if( i % 3 == 2 ){
                joint = Beta;
                velocities[ i ] = -velocities[ i ];
            }


            config.execute( Action( Action::Rotate( arm[ i / 3], joint, velocities[ i ] ) ) );
            config.computeMatrices();
        }

        if( ++iterations == max_iterations ){
            return false;
        }
    }

    return true;
}

bool kinematic_rofibot::connect_pseudoinverse( int a, int b, int max_iterations )
{
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector pointing_a = pointing_to( arms[ a ] );
    Vector pointing_b = pointing_to( arms[ b ] );

    Vector x_frame_a = get_global( arms[ a ].back(), 1, { -1, 0, -1, 1 } );
    Vector x_frame_b = get_global( arms[ b ].back(), 1, { -1, 0, -1, 1 } );

    int iterations = 0;
    while( distance( pointing_a, position_b ) > 0.01 || distance( pointing_b, position_a ) > 0.01 ){
        if( pseudoinverse( pointing_b, x_frame_b - pointing_b, position_b - pointing_b, arms[ a ], 1000 ) ) return true;
        position_a = end_effector( arms[ a ] );
        position_b = end_effector( arms[ b ] );

        pointing_a = pointing_to( arms[ a ] );
        pointing_b = pointing_to( arms[ b ] );

        x_frame_a = get_global( arms[ a ].back(), 1, { -1, 0, -1, 1 } );
        x_frame_b = get_global( arms[ b ].back(), 1, { -1, 0, -1, 1 } );
    
        std::cout << IO::toString( config );
        if( pseudoinverse( pointing_a, x_frame_a - pointing_a, position_a - pointing_a, arms[ b ], 1000 ) ) return true;
        std::cout << IO::toString( config );
        position_a = end_effector( arms[ a ] );
        position_b = end_effector( arms[ b ] );

        pointing_a = pointing_to( arms[ a ] );
        pointing_b = pointing_to( arms[ b ] );

        x_frame_a = get_global( arms[ a ].back(), 1, { -1, 0, -1, 1 } );
        x_frame_b = get_global( arms[ b ].back(), 1, { -1, 0, -1, 1 } );
        
        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }
    return true;
}