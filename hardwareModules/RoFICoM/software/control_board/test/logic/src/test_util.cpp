#include <catch.hpp>
#include <util.hpp>
#include <cstring>

TEST_CASE("util: ViewAs") {
    char buff[16];
    memset( buff, 0, 16 );
    for ( int i = 0; i != 16; i++ )
        viewAs< uint8_t >( buff + i ) = 42 + i;
    for ( int i = 0; i != 16; i++ )
        REQUIRE( buff[ i ] == 42 + i );
}