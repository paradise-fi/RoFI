#include "rofiBot.hpp"

// executable should probably be separated later
int main( int argc, char** argv )
{
    bool fixed = false;
    std::string path;
    strategy s = strategy::fabrik;
    bool reach = false;
    bool connect = false;
    std::pair< int, int > to_connect = { 0, 1 };
    Vector goal = { 0, 0, 0, 1 };

    for( int i = 0; i < argc; ++i ){
        std::string arg = argv[ i ];
        if( arg == "-f" || arg == "--fixed" ){
            fixed = true;
        } else if( arg == "-r" || arg == "--reach" ){
            reach = true;
            for( int j = 0; j < 3; ++j ){
                std::stringstream ss( argv[ i + j + 1 ] );
                ss >> goal[ j ];
            }
        } else if( arg == "-c" || arg == "--connect" ){
            connect = true;
            to_connect.first = std::stoi( argv[ i + 1 ] );
            to_connect.second = std::stoi( argv[ i + 2 ] );
        } else if( arg == "-i" || arg == "--input" ){
            path = argv[ i + 1 ];
        } else if( arg == "-ccd" ){
            s = strategy::ccd;
        }

    }
    rofi_bot bot( path, fixed );
    if( reach ){
        std::cerr << std::boolalpha << bot.reach< strategy::ccd >( goal ) << '\n';
    }
    if( connect ){
        std::cerr << std::boolalpha << bot.connect< strategy::ccd >( to_connect.first, to_connect.second ) << '\n';
    }
    std::cout << IO::toString( bot.get_config() );
    return 0;
}

rofi_bot::rofi_bot( Configuration new_config, bool fixed, Vector position )
                    : config( new_config ), center( position ), fixed( fixed )
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

rofi_bot::rofi_bot( std::string file_name, bool fixed )
{
    std::ifstream input( file_name );
    if( !input.is_open() ){
        std::cerr << "Couldn't open file \'" << file_name << "\'" << std::endl;
        return;
    }

    Configuration new_config;
    Vector center({ 0, 0, 0, 1 });
    std::string line;
    if( std::getline( input, line ) ){
        char identifier;
        std::stringstream linestream( line );
        linestream >> identifier;
        linestream >> center.at( 0 ) >> center.at( 1 ) >> center.at( 2 );
    }
    input.seekg( 0 );
    IO::readConfiguration( input, new_config );

    *this = rofi_bot( new_config, fixed, center );
}

bool rofi_bot::connect_ccd( int a, int b, int max_iterations )
{
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector goal_a = pointing_to( arms[ a ] );
    Vector goal_b = pointing_to( arms[ b ] );

    int iterations = 0;
    while( distance( goal_a, position_b ) > 0.01 || distance( goal_b, position_a ) > 0.01 ){
        ccd( position_b, { 0.0, 0.0, 0.0 }, arms[ a ], 1 );
        ccd( position_a, { 0.0, 0.0, 0.0 }, arms[ b ], 1 );
        position_a = end_effector( arms[ a ] );
        position_b = end_effector( arms[ b ] );

        goal_a = pointing_to( arms[ a ] );
        goal_b = pointing_to( arms[ b ] );
        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }

    return true;
}

bool rofi_bot::connect_fabrik( int a, int b, int max_iterations ){
    config.computeMatrices();

    Vector position_a = end_effector( arms[ a ] );
    Vector position_b = end_effector( arms[ b ] );

    Vector pointing_a = pointing_to( arms[ a ] );
    Vector pointing_b = pointing_to( arms[ b ] );

    /* If the arms face outwards, the algorithm doesn't seem to find a solution.
     * Therefore, rotate the tips towards the other arm */
    if( distance( pointing_a, get_global( arms[ b ].front(), 0 ) ) > arms[ a ].size() ){
        for( Joint joint : { Gamma, Alpha } ){
            rotate_to( position_a, get_global( arms[ b ].front(), 0 ), arms[ a ].back(), joint );
            rotate_to( position_b, get_global( arms[ a ].front(), 0 ), arms[ b ].back(), joint );
        }

        position_a = end_effector( arms[ a ] );
        position_b = end_effector( arms[ b ] );

        pointing_a = pointing_to( arms[ a ] );
        pointing_b = pointing_to( arms[ b ] );

    }
 
    int iterations = 0;
    while( distance( position_a, pointing_b ) > 0.01 || distance( position_b, pointing_a ) > 0.01 ){

        fabrik( pointing_a, position_a, arms[ b ], 5 );

        position_b = end_effector( arms[ b ] );
        pointing_b = pointing_to( arms[ b ] );

        fabrik( pointing_b, position_b, arms[ a ], 5 );

        position_a = end_effector( arms[ a ] );
        pointing_a = pointing_to( arms[ a ] );

        if( ++iterations == max_iterations ){
            std::cerr << "Couldn't connect\n";
            return false;
        }
    }
    return true;
}

// currently made to work for connection, might need to separate the two cases
bool rofi_bot::ccd( const Vector& goal, const std::vector< double >& rotation,
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


bool rofi_bot::rotate_to( const Vector& end_pos, const Vector& end_goal, int i, Joint joint, bool check_validity, bool round )
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

    return rotate_if_valid( i, joint, rotation );
}

bool rofi_bot::rotate_if_valid( int module, Joint joint, double angle )
{
    config.execute( Action( Action::Rotate( module, joint, angle ) ) );
    if( !config.isValid() ){
        config.execute( Action( Action::Rotate( module, joint, -angle ) ) );
        config.computeMatrices();
        return false;
    }
    return true;
}

bool rofi_bot::fabrik( const Vector& goal, const Vector& pointing, const chain& arm, int max_iterations ){
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

    /* To make sure I can rotate the connector as I need to, I set the
     * position of the second last joint opposite to the point I want to
     * face */
    Vector second_goal = goal - ( pointing - goal );

    int iterations = 0;
    while( distance( get_global( arm.back(), 1 ), goal ) > 0.01 ||
           distance( get_global( arm.back(), 0 ), second_goal ) > 0.01 ){
        
        /* save the last position and end the algorithm if it hasn't changed at all */
        Vector last_position = get_global( arm.back(), 1 );
        
        positions.back() = goal;
        positions[ positions.size() - 2 ] = second_goal;

        // old style for loops for easier access to the next element

        /* Forward reaching */
        for( size_t i = positions.size() - 2; i--; ){
            Vector direction = positions[ i + 2 ] - positions[ i + 1 ];
 
            /* The limit set on RoFIs: every other joint is projected on the
             * line between the next and previous one, because there is no 
             * rotation between two modules */
            if( i % 2 == 1 ){
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
            if( distance( positions[ i + 2 ], positions[ i ] ) < std::sqrt( 2.0 ) ){
                direction = positions[ i + 1 ] - positions[ i - 1 ];
                positions[ i ] = rotate( M_PI, direction ) * positions[ i ];
            }
        }

        positions[ 0 ] = base;
        positions[ 1 ] = second;

        /* Backward reaching, this time actually move the links */
        for( size_t i = 1; i < positions.size() - 1; ++i ){
            Joint current = i % 2 == 0 ? Alpha : Beta;
            
            Vector next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
            if( current == Beta ){
                rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], Gamma, false );
            }
            next_position = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
            rotate_to( next_position, positions[ i + 1 ], arm[ i / 2 ], current, false );

            positions[ i + 1 ] = get_global( arm[ ( i + 1 ) / 2 ], ( i + 1 ) % 2 );
        }
        if( ++iterations == max_iterations || equals( last_position, get_global( arm.back(), 1 ) ) ){
            return false;
        }
    }

    return true;
}
