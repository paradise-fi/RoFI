#include <z3++.h>
#include "smtReconfig.hpp"
#include <IO.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <fmt/format.h>
#include <cmath>
#include <cstdlib>
#include <cxxopts.hpp>

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

struct YicesProcSolver {
    YicesProcSolver( z3::solver& s ): _solver( s ) {}

    z3::check_result check() {
        std::ofstream outputFile( "/tmp/yicesFormula.smt2" );
        outputFile << _solver.to_smt2() << "\n(get-model)\n";
        outputFile.close();

        int r = system( "sed -i \"2i (set-logic QF_NRA )\" /tmp/yicesFormula.smt2" );
        if ( r )
            return z3::unknown;
        r = system( "yices-smt2 /tmp/yicesFormula.smt2 > /tmp/yicesOutput.txt" );
        if ( r )
            return z3::unknown;
        std::ifstream in( "/tmp/yicesOutput.txt" );
        std::string result;
        std::getline( in, result );
        if ( result.find( "unknown" ) == 0 )
            return z3::sat;
        if ( result.find( "unsat" ) == 0 )
            return z3::unsat;
        // Process model
        _model = {};
        std::string modelLine;
        while ( getline( in, modelLine ) )
            _parseModelLine( modelLine );
        return z3::sat;
    }

    void _parseModelLine( const std::string& line ) {
        std::istringstream s( line );
        std::string prefix;
        s >> prefix;
        if ( prefix != "(=" )
            return;
        std::string varName;
        s >> varName;
        std::string value;
        std::getline( s, value );
        _model.insert( { varName, strip( value ) } );
    }

    static std::string strip( std::string s ) {
        auto end = std::remove_if( s.begin(), s.end(), []( char c ) {
            return isspace( c ) || c == '(' || c == ')';
        } );
        s.erase( end, s.end() );
        return s;
    }

    std::string getString( const z3::expr& var ) const {
        auto varName = var.decl().name().str();
        auto valIt = _model.find( varName );
        if ( valIt == _model.end() )
            throw std::runtime_error( "Variable not found" );
        return valIt->second;
    }

    bool getBool( const z3::expr& var ) const {
        auto val = getString( var );
        if ( val == "true" )
            return true;
        if ( val == "false" )
            return false;
        throw std::runtime_error( "Not a bool value" );
    }

    float getFloat( const z3::expr& var ) const {
        auto val = getString( var );
        return std::stof( val );
    }

    z3::solver& _solver;
    std::map< std::string, std::string > _model;
};

float radToDeg( float val ) {
    return 180.0 * val / M_PI;
}

float reconstruct( const rofi::smtr::SinCosAngle& angle,
    const z3::model& model )
{
    float sin = std::stof( model.eval( angle.sin ).get_decimal_string( 5 ) );
    float cos = std::stof( model.eval( angle.cos ).get_decimal_string( 5 ) );
    return atan2( sin, cos );
}

float reconstruct( const rofi::smtr::SinCosAngle& angle,
    const YicesProcSolver& solver )
{
    float sin = solver.getFloat( angle.sin );
    float cos = solver.getFloat( angle.cos );
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
        cfg.addModule( radToDeg( alpha ), radToDeg( beta ), radToDeg( gamma ), i );
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

Configuration reconstruct( const rofi::smtr::SmtConfiguration& smtCfg,
    const YicesProcSolver& solver )
{
    using namespace rofi::smtr;

    Configuration cfg;
    for ( int i = 0; i != smtCfg.modules.size(); i++ ) {
        float alpha = reconstruct( smtCfg.modules[ i ].alpha, solver );
        float beta = reconstruct( smtCfg.modules[ i ].beta, solver );
        float gamma = reconstruct( smtCfg.modules[ i ].gamma, solver );
        cfg.addModule( radToDeg( alpha ), radToDeg( beta ), radToDeg( gamma ), i );
    }

    for ( auto [ m, ms, n, ns ] : allShoePairs( smtCfg.modules.size() ) ) {
        for ( auto [ mc, nc, o ] : allShoeConnections() ) {
            const z3::expr& conn = smtCfg.connection( m, ms, mc, n, ns, nc, o );
            if ( !solver.getBool( conn ) )
                continue;
            cfg.addEdge( Edge{ m, ms, mc, o, nc, ns, n } );
        }
    }

    return cfg;
}

z3::check_result yicesSolve( z3::solver& s ) {
    std::ofstream outputFile( "/tmp/yicesFormula.smt2" );
    outputFile << s.to_smt2() << "\n(get-model)\n";
    outputFile.close();

    int r = system( "sed -i \"2i (set-logic QF_NRA )\" /tmp/yicesFormula.smt2" );
    r = system( "yices-smt2 /tmp/yicesFormula.smt2 > /tmp/yicesOutput.txt" );
    if ( r )
        return z3::unknown;
    std::ifstream in( "/tmp/yicesOutput.txt" );
    std::string result;
    std::getline( in, result );
    if ( result.find( "sat" ) == 0 )
        return z3::sat;
    return z3::unsat;
}

auto buildFormula( rofi::smtr::Context& ctx, Configuration& initial,
    Configuration& target, int length )
{
    auto [phi, cfgs ] = rofi::smtr::reconfig( ctx, length, initial, target );
    if ( ctx.cfg.simplify )
        phi = phi.simplify();
    return std::make_pair( phi, cfgs );
}

int runSmt( Configuration& init, Configuration& target, int length,
    rofi::smtr::Parameters params )
{
    StopWatch formulaTime;
    formulaTime.start();
    rofi::smtr::Context ctx( params );
    z3::solver s( ctx.ctx, "QF_NRA" );
    auto [ phi, cfgs ] = buildFormula( ctx, init, target, length );
    s.add( phi );
    formulaTime.stop();

    std::cout << s.to_smt2() << "\n(get-model)\n";
    std::cerr << "Formula build-time: " << formulaTime.ms() << " ms\n";
    return 0;
}

int runReconfig( Configuration& init, Configuration& target, int length,
    rofi::smtr::Parameters params )
{
    using namespace IO;
    for ( int i = 2; i != length; i++ ) {
        std::cerr << "Trying length " << i << "\n";
        StopWatch formulaTime;
        formulaTime.start();
        rofi::smtr::Context ctx( params );
        z3::solver s( ctx.ctx, "QF_NRA" );
        auto [ phi, cfgs ] = buildFormula( ctx, init, target, length );
        s.add( phi );
        formulaTime.stop();

        std::ofstream outputFile( fmt::format( "/tmp/formula_{}.smt2", i ) );
        outputFile << s.to_smt2() << "\n";
        outputFile.close();

        StopWatch solverTime;
        solverTime.start();
        std::cerr << "    Formula build-time: " << formulaTime.ms() << " ms\n";
        // auto res = s.check();
        YicesProcSolver ys( s );
        auto res = ys.check();
        solverTime.stop();
        std::cerr << "    Result: " << res << " in " << solverTime.ms() << " ms\n";

        if ( res == z3::unsat )
            continue;

        // z3::model model = s.get_model();

        // for ( auto cfg : cfgs ) {
        //     std::cout << toString( reconstruct( cfg, model ) ) << "\n";
        // }
        for ( auto cfg : cfgs ) {
            std::cout << toString( reconstruct( cfg, ys ) ) << "\n";
        }
        break;
    }
    return 0;
}

template < typename Args >
std::string getCommand( Args& args ) {
    if ( !args.count("positional") )
        return "";
    auto &v = args[ "positional" ].template as< std::vector< std::string > >();
    if ( v.size() < 1 )
        return "";
    return v[ 0 ];
}

int main( int argc, char **argv ) {
    cxxopts::Options options( argv[ 0 ], " - RoFI SMT reconfig" );

    std::string initial, final;
    int length = -1;
    options.add_options()
        ( "positional", "[smt|reconfig]",
            cxxopts::value< std::vector< std::string > >() )
        ( "f, final", "file with target configuration",
            cxxopts::value< std::string >( final ) )
        ( "i, initial", "file with initial configuration",
            cxxopts::value< std::string >( initial ) )
        ( "l, length", "either upper bound for the smt command or the formula length",
            cxxopts::value< int >( length ), "N" )
        ( "s, simplify", "simplify the formula")
        ( "shoeLimitConstrain", "" )
        ( "connectorLimitConstrain", "" )
        ( "90degReconfig", "");
    options.parse_positional( { "positional" } );

    auto args = options.parse( argc, argv );

    using namespace IO;
    Configuration init, target;

    std::ifstream initFile( initial );
    if ( !IO::readConfiguration( initFile, init ) ) {
        std::cerr << "Invalid init file\n";
        return 1;
    }

    std::ifstream targetFile( final );
    if ( !IO::readConfiguration( targetFile, target ) ) {
        std::cerr << "Invalid target file\n";
        return 1;
    }

    rofi::smtr::Parameters params;
    params.simplify = args.count( "simplify" );
    params.connectorLimitConstrain = args.count( "connectorLimitConstrain" );
    params.shoeLimitConstain = args.count( "shoeLimitConstrain" );
    if ( args.count( "90degReconfig" ) ) {
        params.stepSize = rofi::smtr::Parameters::StepSize::Step90;
    }

    auto command = getCommand( args );
    if ( command == "smt" ) {
        if ( length == -1 ) {
            std::cerr << "--lenght or -l is required but was not specified\n";
            return 1;
        }
        return runSmt( init, target, length, params );
    }
    if ( command == "reconfig" ) {
        return runReconfig( init, target, length, params );
    }

    if ( command.empty() )
        std::cerr << "No command specify. Specify 'smt' or 'reconfig'\n";
    else
        std::cerr << "Invalid command '" << command << "' specified.\n";
    return 1;
}