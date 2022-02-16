#include <fabrik.hpp>


Matrix generateTarget( Configuration config ){
    tentacleMonster tm( config );
    auto& modules = tm.config.getModules();
    do {
        for( auto& [ id, module ] : modules ){
            int r = rand() % 100 - 50;
            //module.setJoint( Alpha, r );
            tm.setJoint( id, Alpha, to_rad( r ) );
            r = rand() % 360;
            //module.setJoint( Gamma, r );
            tm.setJoint( id, Gamma, to_rad( r ) );
            if( !config.isValid() )
                tm.setJoint( id, Gamma, 0.0 );
            r = rand() % 100 - 50;
            tm.setJoint( id, Beta, to_rad( r ) );
            if( !config.isValid() )
                tm.setJoint( id, Beta, 0.0 );
        }
    } while( !config.isValid() );

    tm.config.computeMatrices();
    auto mat = tm.config.getMatrices();
    return mat[ mat.size() ][1];
}

int main(){
    srand( time( NULL ) );
    std::ifstream input( "data/configurations/kinematics/3zz_test.rofi" );
    Configuration config;
    IO::readConfiguration( input, config );
    int count = 0;
    int correct = 0;
    Matrix mat;
    Matrix act;
    //tentacleMonster tm( config );
    for( int i = 0; i < 100; ++i ){
        mat = generateTarget( config );

        tentacleMonster tm( config );
        bool res = tm.fabrik( tm.tentacles[ 0 ], mat * rotate( M_PI, Z ) * rotate( M_PI, X ) );
        count += res;
        tm.config.computeMatrices();
        auto actual = tm.config.getMatrices();
        act = actual[ actual.size() ][ 1 ];
        bool actres = arma::approx_equal( mat, actual[ actual.size() ][ 1 ], "absdiff", 0.002 );
        correct += actres;

    }
    std::cout << count << '\n' << correct << '\n';/**/
 }
