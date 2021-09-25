#include "umpad.hpp"

namespace rofi {

Rofibot buildUMpad( int n ) {
    return buildUMpad( n, n );
}

Rofibot buildUMpad( int n, int m ) {
    assert( n > 0 && "NMPad has to have positive dimensions" );

    Rofibot bot;
    std::vector< ModuleId > modules;
    modules.reserve( n * m );

    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < m; j++ ) {
            auto& md = bot.insert( buildUniversalModule( 0_deg, 0_deg, 90_deg ) );
            if ( j > 0 )
                connect( md.connector( 4 ), bot.getModule( modules.back() )->connector( 3 ), Orientation::South );
            
            if ( i > 0 )
                connect( md.connector( 1 ), bot.getModule( modules[ (i - 1) * m + j ] )->connector( 0 ), Orientation::South );

            modules.push_back( md.id );
        }
    }

    return bot;
}

} // namespace rofi
