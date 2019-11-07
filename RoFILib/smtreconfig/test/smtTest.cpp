#include <catch2/catch.hpp>
#include <smt.hpp>

int setBits( uint32_t n )  {
    int count = 0;
    while ( n ) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

TEST_CASE("At most one") {
    for ( int pattern = 0; pattern != 255; pattern++ ) {
        z3::context ctx;
        std::vector< z3::expr > vars;
        for ( int i = 0; i != 8; i++ ) {
            vars.push_back( ctx.bool_const( std::to_string( i ).c_str() ) );
        }

        z3::solver s( ctx );
        s.add( smt::atMostOne( ctx, vars ) );

        for ( int i = 0; i != 8; i++ ) {
            if ( ( pattern >> i ) & 1 )
                s.add( vars[ i ] );
            else
                s.add( !vars[ i ] );
        }

        if ( setBits( pattern ) <= 1 ) {
            CAPTURE( pattern );
            REQUIRE( s.check() == z3::sat );
        }
        else {
            CAPTURE( pattern );
            REQUIRE( s.check() == z3::unsat );
        }
    }
}