#include "kinematics.cpp"

int main( int argc, char** argv )
{
    bool fixed = false;
    std::string path;
    options opt;

    strategy s = strategy::fabrik;
    bool reach = false;
    bool connect = false;
    std::pair< int, int > to_connect = { 0, 1 };
    Vector goal = { 0, 0, 0, 1 };
    std::vector< double > rotation = { 0, 0, 0 };
    std::ifstream targets;
    std::ofstream results;

    for( int i = 1; i < argc; ++i ){
        std::string arg = argv[ i ];
        if( arg == "-f" || arg == "--fixed" ){
            fixed = true;
        } else if( arg == "-r" || arg == "--reach" ){
            reach = true;
            if( argc < i + 6 ){
                std::cerr << "Not enough coordinates given\n";
                return 1;
            }
            for( int j = 0; j < 3; ++j ){
                std::stringstream ss( argv[ ++i ] );
                try {
                    std::stod( ss.str() );
                } catch( std::exception& ) {
                    std::cerr << "Invalid position coordinate: " << ss.str() << '\n';
                    return 1;
                }
                ss >> goal[ j ];
            }

            for( int j = 0; j < 3; ++j ){
                std::stringstream ss( argv[ ++i ] );
                try {
                    std::stod( ss.str() );
                } catch( std::exception& ) {
                    std::cerr << "Invalid rotation: " << ss.str() << '\n';
                }
                ss >> rotation[ j ];
            }

        } else if( arg == "-c" || arg == "--connect" ){
            connect = true;
            to_connect.first = std::stoi( argv[ ++i ] );
            to_connect.second = std::stoi( argv[ ++i ] );
        } else if( arg == "-res" || arg == "--results" ){
            results.open( argv[ ++i ] );
            if( !results.is_open() ){
                std::cerr << "Couldn't open file " << argv[ i ] << '\n';
                return 1;
            }
        } else if( arg == "-t" || arg == "--targets" ){
            targets.open( argv[ ++i ] );
            if( !targets.is_open() ){
                std::cerr << "Couldn't open file " << argv[ i ] << '\n';
                return 1;
            }
        } else if( arg == "-i" || arg == "--input" ){
            path = argv[ ++i ];
        } else if( arg == "-ccd" ){
            s = strategy::ccd;
        } else if( arg == "-pi" ){
            s = strategy::pseudoinverse;
        } else if( arg == "-fabrik" ){
            s = strategy::fabrik;
        } else if( arg == "-v" || arg == "--verbose" ){
            opt.verbose = true;
        } else if( arg == "-a" || arg == "--animate" ){
            opt.animate = true;
        } else {
            std::cerr << "Unrecognized command line option: " << arg << '\n';
            return 1;
        }

    }
    kinematic_rofibot bot( path, fixed, opt );

    bool result;
    if( !targets.is_open() ){
        if( reach ){
            result = bot.reach( goal, rotation, s ) << '\n';
        }
        if( connect ){
            result = bot.connect( s, to_connect.first, to_connect.second ) << '\n';
        }
        if( results.is_open() )
            results << std::boolalpha << result << '\n';
        else
            std::cerr << std::boolalpha << result << '\n';
    } else {
        Configuration reset = bot.get_config();
        std::string line;
        while( getline( targets, line ) ){
            std::string method;
            std::stringstream ss( line );
            ss >> method;
            if( method == "r" ){
                ss >> goal[ 0 ] >> goal[ 1 ] >> goal[ 2 ]
                    >> rotation[ 0 ] >> rotation[ 1 ] >> rotation[ 2 ];
                result = bot.reach( goal, rotation, s ) << '\n';
            } else if( method == "c" ){
                ss >> to_connect.first >> to_connect.second;
                result = bot.connect( s, to_connect.first, to_connect.second ) << '\n';
            } else {
                std::cerr << "Wrong target format\n";
                return 1;
            }

            if( results.is_open() )
                results << std::boolalpha << result << '\n';
            else
                std::cerr << std::boolalpha << result << '\n';
        }
    }
    std::cout << IO::toString( bot.get_config() );
    return 0;
}