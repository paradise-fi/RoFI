#include <z3++.h>
#include "smtReconfig.hpp"
#include <IO.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <fmt/format.h>
#include <cmath>

struct StopWatch {
    void start() {
        _start = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        _stop = std::chrono::high_resolution_clock::now();
    }

    int ms() {
        using namespace std::chrono;
        return duration_cast< milliseconds >( _stop - _start ).count();
    }

    std::chrono::high_resolution_clock::time_point _start, _stop;
};

float reconstruct( const rofi::smtr::SinCosAngle& angle,
    const z3::model& model )
{
    float sin = std::stof( model.eval( angle.sin ).get_decimal_string( 5 ) );
    float cos = std::stof( model.eval( angle.cos ).get_decimal_string( 5 ) );
    return atan2( sin, cos );
}

Configuration reconstruct( const rofi::smtr::SmtConfiguration& smtCfg,
    const z3::model& model )
{
    using namespace rofi::smtr;

    Configuration cfg;
    for ( int i = 0; i != smtCfg.modules.size(); i++ ) {
        float alpha = reconstruct( smtCfg.modules[ i ].alpha, model );
        float beta = reconstruct( smtCfg.modules[ i ].beta, model );
        float gamma = reconstruct( smtCfg.modules[ i ].gamma, model );
        cfg.addModule( alpha, beta, gamma, i );
    }

    for ( auto [ m, ms, n, ns ] : allShoePairs( smtCfg.modules.size() ) ) {
        for ( auto [ mc, nc, o ] : allShoeConnections() ) {
            const z3::expr& conn = smtCfg.connection( m, ms, mc, n, ns, nc, o );
            bool connected = model.eval( conn ).is_true();
            if ( !connected )
                continue;
            cfg.addEdge( Edge{ m, ms, mc, o, nc, ns, n } );
        }
    }

    return cfg;
}

int main( int argc, char **argv ) {
    using namespace IO;
    Configuration init, target;

    std::ifstream initFile( argv[ 1 ] );
    if ( !readConfiguration( initFile, init ) ) {
        std::cerr << "Invalid init file\n";
        return 1;
    }

    std::ifstream targetFile( argv[ 2 ] );
    if ( !readConfiguration( targetFile, target ) ) {
        std::cerr << "Invalid target file\n";
        return 1;
    }

    for ( int i = 2; i != 50; i++ ) {
        std::cerr << "Trying length " << i << "\n";
        StopWatch formulaTime;
        formulaTime.start();
        rofi::smtr::Context ctx;
        z3::solver s( ctx.ctx );
        auto [phi, cfgs ] = rofi::smtr::reconfig( ctx, i, init, target );
        s.add( phi );
        formulaTime.stop();

        std::ofstream outputFile( fmt::format( "/tmp/formula_{}.smt2", i ) );
        outputFile << s.to_smt2() << "\n";
        outputFile.close();

        StopWatch solverTime;
        solverTime.start();
        std::cerr << "    Formula built in " << formulaTime.ms() << " ms\n";
        auto res = s.check();
        solverTime.stop();
        std::cerr << "    Result: " << res << " in " << solverTime.ms() << " ms\n";

        if ( res == z3::unsat )
            continue;

        z3::model model = s.get_model();

        for ( auto cfg : cfgs ) {
            std::cout << toString( reconstruct( cfg, model ) ) << "\n";
        }
        break;
    }
}