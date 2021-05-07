#include "kinematics.hpp"

kinematic_rofibot::kinematic_rofibot( Configuration new_config, bool fixed, options opt )
                    : config( new_config ), fixed( fixed ), opt( opt )
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
                arms.emplace_back( chain{ } );
                auto edge = edges.front();
                while( ( edges = config.getEdges( current, exclude ) ).size() == 1 ){
                    arms.back().push_front( current );
                    exclude = { current };
                    current = edges.front().id2();
                }
            }
        }
    }
    
    config.computeMatrices();

};

kinematic_rofibot::kinematic_rofibot( std::string file_name, bool fixed, options opt ) : opt( opt )
{
    std::ifstream input( file_name );
    if( !input.is_open() ){
        std::cerr << "Couldn't open file \'" << file_name << "\'" << std::endl;
        return;
    }

    Configuration new_config;

    IO::readConfiguration( input, new_config );

    *this = kinematic_rofibot( new_config, fixed, opt );
}

bool kinematic_rofibot::connect_ccd( int a, int b, int max_iterations )
{
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector goal_a = pointing_to( arms[ a ] );
    Vector goal_b = pointing_to( arms[ b ] );

    Vector x_a = get_global( arms[ a ].back(), 1, { -1, 0, -1, 1 } );
    Vector x_b = get_global( arms[ b ].back(), 1, { -1, 0, -1, 1 } );

    Vector y_a = get_global( arms[ a ].back(), 1, { 0, -1, -1, 1 } );
    Vector y_b = get_global( arms[ b ].back(), 1, { 0, -1, -1, 1 } );

    int iterations = 0;
    while( distance( goal_a, position_b ) > 0.01 || distance( goal_b, position_a ) > 0.01 ){
        ccd( goal_b, x_b - goal_b, y_b - goal_b, position_b - goal_b, arms[ a ], 10 );
        position_a = end_effector( arms[ a ] );
        goal_a = pointing_to( arms[ a ] );
        x_a = get_global( arms[ a ].back(), 1, { -1, 0, -1, 1 } );
        y_a = get_global( arms[ a ].back(), 1, { 0, -1, -1, 1 } );
    
        if( opt.animate )
            std::cout << IO::toString( config );
        ccd( goal_a, x_a - goal_a, y_a - goal_a, position_a - goal_a, arms[ b ], 10 );
        position_b = end_effector( arms[ b ] );
        goal_b = pointing_to( arms[ b ] );
        x_b = get_global( arms[ b ].back(), 1, { -1, 0, -1, 1 } );
        y_b = get_global( arms[ b ].back(), 1, { 0, -1, -1, 1 } );
    
        if( opt.animate )
            std::cout << IO::toString( config );

        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }
    if( opt.verbose ){
        std::cerr << "pointing_b:\n" << goal_b << "a:\n" << position_a << "pointing_a:\n" << goal_a << "b:\n" << position_b;
    }
    return true;
}

bool kinematic_rofibot::connect_fabrik( int a, int b, int max_iterations ){
    config.computeMatrices();

    chain arm = arms[ a ];
    arm.insert( arm.end(), arms[ b ].rbegin(), arms[ b ].rend() );
    Vector b_begin = get_global( arms[ b ].front(), 0 );
    Vector b_z = get_global( arms[ b ].front(), 0, { 0, 0, -1, 1 } );
    Vector b_y = get_global( arms[ b ].front(), 0, { 0, 1, 0, 1 } );
    fabrik( b_begin, b_y - b_begin, b_z - b_begin, arm, 1000, arms[ a ].size() );

    Edge e = { arms[ a ].back(), B, ZMinus, North, ZMinus, B, arms[ b ].back() };
    return config.execute( Action( Action::Reconnect( true, e ) ) );
}

bool kinematic_rofibot::ccd( const Vector& goal, const Vector& x_frame, const Vector& y_frame,
                             const Vector& z_frame, const chain& arm, int max_iterations )
{
    double wp = 1.0, wa = 1.0;
    double k1, k2, k3;

    double rotation;

    Vector end_position = end_effector( arm );

    Vector z_goal = goal + z_frame;
    Vector y_goal = goal + y_frame;
    Vector x_goal = goal + x_frame;

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

                // Vector current_x = { -get_matrix( arm.back(), 1 ).at( 0, 0 ),
                //                      -get_matrix( arm.back(), 1 ).at( 0, 1 ),
                //                      -get_matrix( arm.back(), 1 ).at( 0, 2 ),
                //                      1.0 };
                // Vector current_y = { get_matrix( arm.back(), 1 ).at( 1, 0 ),
                //                      get_matrix( arm.back(), 1 ).at( 1, 1 ),
                //                      get_matrix( arm.back(), 1 ).at( 1, 2 ),
                //                      1.0 };
                // Vector current_z = { -get_matrix( arm.back(), 1 ).at( 2, 0 ),
                //                      -get_matrix( arm.back(), 1 ).at( 2, 1 ),
                //                      -get_matrix( arm.back(), 1 ).at( 2, 2 ),
                //                      1.0 };

                current_x = current_x / magnitude( current_x );
                current_y = current_y / magnitude( current_y );
                current_z = current_z / magnitude( current_z );

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

                k3 = axis * ( wp * cross_product( local_end, local_goal )  +
                              wa * ( cross_product( current_x, x_frame ) +
                                     cross_product( current_y, y_frame ) +
                                     cross_product( current_z, z_frame ) ) );

                double candidate = std::atan( k3 / ( k2 - k1 ) );

                auto second_derivative = [=]( double rot ) {
                    return ( k1 - k2 ) * std::cos( rot ) + k3 * ( -1 ) * std::sin( rot );
                };
                auto equation = [=]( double rot ){
                    return k1 * (1 - std::cos( rot ) + k2 * std::cos( rot ) + k3 * std::sin( rot ) );
                };

                if( second_derivative( candidate ) < 0 && candidate + M_PI < M_PI &&
                    second_derivative( candidate + M_PI ) < 0 )
                {
                    rotation = to_deg( equation( candidate ) > equation( candidate + M_PI ) ?
                                        candidate : candidate + M_PI );
                } else if( second_derivative( candidate ) < 0 && candidate - M_PI > -M_PI &&
                           second_derivative( candidate - M_PI ) < 0 )
                {
                    rotation = to_deg( equation( candidate ) > equation( candidate - M_PI ) ?
                                        candidate : candidate - M_PI );
                } else if( second_derivative( candidate ) < 0 ){
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

                rotate_to( pointing_to( arm ), end_position + z_frame, arm.back(), Gamma );
                rotate_to( pointing_to( arm ), end_position + z_frame, arm.back(), Beta );
            }
        }
        
        if( opt.animate )
            std::cout << IO::toString( config );

        if( ++iterations == max_iterations ){
            if( opt.verbose ){
                std::cerr << IO::toString( get_matrix( arm.back(), 1 ) );
            }
            return false;
        }
    }

    if( opt.verbose ){
        std::cerr << IO::toString( get_matrix( arm.back(), 1 ) );
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

    if( !check_validity ){
        config.execute( Action( Action::Rotate( i, joint, rotation ) ) );
        config.computeMatrices();
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
                                const chain& arm, int max_iterations, int a_length )
{
    assert( rotation.size() == 3 );
    assert( max_iterations > 0 );

    std::vector< Vector > positions;
    if( a_length == -1 )
        for( int i : arm ){
            for( int j : { Alpha, Beta } ){
                positions.emplace_back( get_global( i, j ) );
            }
        }
    else {
        for( int i = 0; i < arm.size(); ++i ){
            if( i < a_length )
                for( int j : { Alpha, Beta } ){
                    positions.emplace_back( get_global( arm[ i ], j ) );
                }
            else
            {
                for( int j : { Beta, Alpha } ){
                    positions.emplace_back( get_global( arm[ i ], j ) );
                }
            }
            
        }
    }

    Vector base = positions[ 0 ];

    auto respects_limit = [&]() {
        Vector normal = cross_product( z_frame, y_frame );
        Vector dir = positions[ positions.size() - 2 ] - positions.back();

        Vector projected = positions[ positions.size() - 2 ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;
        return equals( projected, positions[ positions.size() - 2 ]);
    };
    int iterations = 0;
    /* At the start of the connect method, the final point is equal to the goal;
     * a different enter condition is needed */
    bool enter = true;
    while( /*distance( end_effector( arm ), goal ) > 0.01 ||
           distance( pointing_to( arm ), goal + z_frame ) > 0.01*/
            distance( positions.back(), goal ) > error || enter ||
            !respects_limit() )
    {
        enter = false;
        if( opt.animate )
            std::cout << IO::toString( config );
        
        /* save the last position and end the algorithm if it hasn't changed at all */
        Vector last_position = positions.back();
        
        positions.back() = goal;

        // old style for loops for easier access to the next element

        /* Forward reaching */
        for( size_t i = positions.size() - 1; i--; ){
            /* The limit set on RoFIs: every other joint is projected on the
             * line between the next and previous one, because there is no 
             * rotation between two modules */
            if( i == positions.size() - 2 ){
                Vector normal = cross_product( z_frame, y_frame );

                Vector dir = positions[ i ] - positions[ i + 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;
            } else if( i % 2 == 1 ){
                Vector direction = positions[ i + 2 ] - positions[ i + 1 ];
                Vector to_base = positions[ i - 1 ] - positions[ i + 1 ];
                Vector normal = cross_product( direction, to_base );

                Vector dir = positions[ i ] - positions[ i + 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;
            }

            /* Sets the joint on the line between its previous position and
             * the next one, so that the link length is one **/
            // std::cerr << "i: " << i << '\n' << "pos_i" << positions[i] << "pos_i1" << positions[ i + 1 ];
            double lambda = 1.0 / distance( positions[ i ], positions[ i + 1 ] );
            positions[ i ] = ( 1.0 - lambda ) * positions[ i + 1 ] + lambda * positions[ i ];

            /* If the new position is too close (doesn't respect the limit of rotation to 90°),
             * reposition it to the other side **/
            Vector prev = i == positions.size() - 2 ? goal + z_frame : positions[ i + 2 ];
            if( distance( prev, positions[ i ] ) < std::sqrt( 2.0 ) ){
                Vector normal = prev - positions[ i + 1 ];
                // positions[ i ] = positions[ i ] - ( normal * ( positions[ i ] - positions[ i + 1 ] ) ) * normal;

                Vector dir = positions[ i ] - positions[ i + 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;

                lambda = 1.0 / distance( positions[ i ], positions[ i + 1 ] );
                positions[ i ] = ( 1.0 - lambda ) * positions[ i + 1 ] + lambda * positions[ i ];
            }
        }

        positions[ 0 ] = base;

        /* Backward reaching stage **/
        for( size_t i = 1; i < positions.size(); ++i ){
            if( i == 1 ){
                Vector normal = cross_product( z_frame, y_frame );
                Vector dir = positions[ i ] - positions[ i - 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;
            } else if( i % 2 == 0 ){
                Vector direction = positions[ i - 2 ] - positions[ i - 1 ];
                Vector to_base = positions[ i + 1 ] - positions[ i - 1 ];
                Vector normal = cross_product( direction, to_base );

                Vector dir = positions[ i ] - positions[ i - 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;

            }

            double lambda = 1.0 / distance( positions[ i ], positions[ i - 1 ] );
            positions[ i ] = ( 1.0 - lambda ) * positions[ i - 1 ] + lambda * positions[ i ];

            Vector prev = i == 1 ? get_global( arm.front(), 0, { 0, 0, -1, 1 } ) : positions[ i - 2 ];

            if( distance( prev, positions[ i ] ) < std::sqrt( 2.0 ) ){
                Vector normal = prev - positions[ i - 1 ];
                Vector dir = positions[ i ] - positions[ i - 1 ];
                positions[ i ] = positions[ i ] - ( ( dir * normal ) / ( normal * normal ) ) * normal;

                lambda = 1.0 / distance( positions[ i ], positions[ i - 1 ] );
                positions[ i ] = ( 1.0 - lambda ) * positions[ i - 1 ] + lambda * positions[ i ];
            }
        }

        if( opt.animate ){
            for( size_t i = 0; i < positions.size() - 1; ++i ){
                Joint current = i % 2 == 0 ? Alpha : Beta;

                Vector next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
                if( current == Beta ){
                    rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], Gamma, true );
                }
                next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
                rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], current, true );
            }
            std::cout << IO::toString( config );
        }

        if( ++iterations == max_iterations || distance( last_position, positions.back() ) < 1e-10 ){
            return false;
        }
    }

    /* Get the joints to the calculated positions */

    Vector next_position;
    /* some tricky positions take more than one iteration to align properly */
    int it = 0;
    /* Get the first (or only) arm in position */
    for( size_t i = 0; i < a_length * 2; ++i ){
        Joint current = i % 2 == 0 ? Alpha : Beta;

        next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
        it = 0;
        while( ( distance ( next_position, positions[ i + 1 ] ) > 1e-10 ) && it < 10 ){
            if( current == Beta ){
                rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], Gamma, false );
            } else if( i != 0 && std::fabs( config.getModule( arm[ ( i - 1 ) / 2 ] ).getJoint( Beta ) ) < 1 ) {
                rotate_to( next_position, positions[ i + 1 ], arm[ ( i - 1 ) / 2 ], Gamma, false );
            }
            next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
            rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], current, false );
            ++it;
        }
    }

    it = 0;
    int last = a_length * 2 - 1;
    Vector position;
    if( last != positions.size() - 1 ){
        position = positions[ last + 1 ];
    } else {
        position = end_effector( arm ) + z_frame;
    }
    while( ( distance ( next_position, positions[ last + 1 ] ) > 1e-10 ) && it < 10 ){
        rotate_to( next_position, position, arm[ i / 2 ], Gamma, false );
        next_position = get_global( arm [ last / 2 ], 1, { 0, 0, -1, 1 } );
        rotate_to( next_position, position, arm[ last / 2 ], Beta, false );
        next_position = get_global( arm [ last / 2 ], 1, { 0, 0, -1, 1 } );
        ++it;
    }

    /* Second arm */
    for( size_t i = positions.size() - 1; i > a_length * 2; --i ){
        Joint current = i % 2 == 0 ? Beta : Alpha;

        Vector next_position = get_global( arm[ ( i - 1 ) / 2 ], ( i ) % 2 );
        it = 0;
        while( ( distance ( next_position, positions[ i + 1 ] ) > 1e-10 ) && it < 10 ){
            if( current == Beta ){
                rotate_to( next_position, positions[ i - 1 ], arm[ i / 2 ], Gamma, false );
            } else if( i != 0 && std::fabs( config.getModule( arm[ ( i + 1 ) / 2 ] ).getJoint( Beta ) ) < 1 ) {
                rotate_to( next_position, positions[ i - 1 ], arm[ ( i + 1 ) / 2 ], Gamma, false );
            }
            next_position = get_global( arm[ ( i - 1 ) / 2 ], ( i ) % 2 );
            rotate_to( next_position, positions[ i - 1 ], arm[ i / 2 ], current, false );
            ++it;
        }
    }
    it = 0;
    i = a_length * 2;
    if( i != positions.size() ){
        next_position = get_global( arm [ i / 2 ], 1, { 0, 0, -1, 1 } );
        while( ( distance ( next_position, positions[ i + 1 ] ) > 1e-10 ) && it < 10 ){
            rotate_to( next_position, positions[ i - 1 ], arm[ i / 2 ], Gamma, false );
            next_position = get_global( arm [ i / 2 ], 1, { 0, 0, -1, 1 } );
            rotate_to( next_position, positions[ i - 1 ], arm[ i / 2 ], Beta, false );
            next_position = get_global( arm [ i / 2 ], 1, { 0, 0, -1, 1 } );
            ++it;
        }
    }

    if( opt.verbose ){
        auto it = arm.begin();
        bool eh = false;
        for( auto p : positions ){
            std::cerr << p << '\n';
            if( eh ){
                std::cerr << "0:\n" << get_global( *it, 0 ) << '\n';
                std::cerr << "1:\n" << get_global( *it, 1 ) << '\n';
                ++it;
                eh = false;
            } else {
                eh = true;
            }
        }
    }       for( int i : arm ){
        }

    if( opt.verbose ){
        std::cerr << IO::toString( get_matrix( arm.back(), 1 ) );
    }
    return true;
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
        if( opt.animate )
            std::cout << IO::toString( config );

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
            if( opt.verbose ){
                std::cerr << IO::toString( get_matrix( arm.back(), 1 ) );
            }
            return false;
        }
    }
    if( opt.verbose ){
        std::cerr << IO::toString( get_matrix( arm.back(), 1 ) );
    }

    return true;
}

bool kinematic_rofibot::connect_pseudoinverse( int a, int b, int max_iterations )
{
    config.computeMatrices();
    // TODO: finish the method
    // Edge toRemove = { 1, B, ZMinus, North, ZMinus, A, 2 };
    // config.execute( Action( Action::Reconnect( false, toRemove ) ) );
    // Edge e = { 1, A, ZMinus, North, ZMinus, B, 3 };
    // config.execute( Action( Action::Reconnect( true, e ) ) );
    return config.isValid();
}