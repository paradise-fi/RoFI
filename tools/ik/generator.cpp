#include <fabrik.hpp>


Matrix generateTarget( Configuration config ){
    auto& modules = config.getModules();
    do {
        for( auto& [ id, module ] : modules ){
            int r = rand() % 100 - 50;
            module.setJoint( Alpha, r );
            r = rand() % 360;
            module.setJoint( Gamma, r );
            if( !config.isValid() )
                module.setJoint( Gamma, 0.0 );
            r = rand() % 100 - 50;
            module.setJoint( Beta, r );
            if( !config.isValid() )
                module.setJoint( Beta, 0.0 );
        }
    } while( !config.isValid() );

    std::cout << IO::toString( config );
    tentacleMonster tm( config );
    return tm.tentacles.front().back().trans * rotate( M_PI, Z ) * rotate( M_PI, X );
    // config.computeMatrices();
    // auto mat = config.getMatrices();
    //return mat[3][1];
}

int main(){
    srand( time( NULL ) );
    std::ifstream input( "data/configurations/kinematics/3zz.rofi" );
    Configuration config;
    IO::readConfiguration( input, config );
    int count = 0;
    for( int i = 0; i < 1; ++i ){
        auto mat = generateTarget( config );
        std::cout << IO::toString( mat );

        tentacleMonster tm( config );
        count += tm.fabrik( tm.tentacles[ 0 ], mat ) ? 1 : 0;
    }
    std::cout << count << '\n';
}
