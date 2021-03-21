#include "rofiBot.hpp"

// executable should probably be separated later
int main( int argc, char** argv )
{
    bool fixed = fixed;
    std::string path;

    for( int i = 0; i < argc; ++i ){
        std::string arg = argv[ i ];
        if( arg == "-f" || arg == "--fixed" ){
            fixed = true;
        } else {
            path = arg;
        }

    }
    rofi_bot bot( path );
    bot.connect();
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
                rotate_to( end_position, goal, *it, j );
                end_position = pointing_to( arm );
            }
        }
        if( ++iterations == max_iterations )
            return false;
    }
    return true;
}


bool rofi_bot::rotate_to( Vector& end_pos, const Vector& end_goal, int i, Joint joint, bool round )
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
        return false;
    }
    return true;
}

