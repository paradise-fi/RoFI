#include <z3++.h>
#include "smtReconfig.hpp"
#include <IO.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <fmt/format.h>

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
        std::cout << "Trying length " << i << "\n";
        StopWatch formulaTime;
        formulaTime.start();
        rofi::smtr::Context ctx;
        z3::solver s( ctx.ctx );
        s.add( rofi::smtr::reconfig( ctx, i, init, target ) );
        formulaTime.stop();

        std::ofstream outputFile( fmt::format( "/tmp/formula_{}.smt2", i ) );
        outputFile << s.to_smt2() << "\n";
        outputFile.close();

        StopWatch solverTime;
        solverTime.start();
        std::cout << "    Formula built in " << formulaTime.ms() << " ms\n";
        auto res = s.check();
        solverTime.stop();
        std::cout << "    Result: " << res << " in " << solverTime.ms() << " ms\n";
    }
}