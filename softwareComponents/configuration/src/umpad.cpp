#include <configuration/bots/umpad.hpp>
#include <configuration/universalModule.hpp>

namespace rofi::configuration {

RofiWorld buildUMpad( int n ) {
    return buildUMpad( n, n );
}

RofiWorld buildUMpad( int n, int m ) {
    assert( n > 0 && m > 0 && "NMPad has to have positive dimensions" );

    ModuleId idCounter = 0;
    RofiWorld world;;
    std::vector< ModuleId > modules;
    modules.reserve( n * m );

    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < m; j++ ) {
            auto& md = world.insert( UniversalModule( idCounter++, 0_deg, 0_deg, 90_deg ) );
            if ( j > 0 )
                connect( md.connectors()[ 4 ], world.getModule( modules.back() )->connectors()[ 3 ]
                       , roficom::Orientation::South );
            
            if ( i > 0 )
                connect( md.connectors()[ 1 ], world.getModule( modules[ (i - 1) * m + j ] )->connectors()[ 0 ]
                       , roficom::Orientation::South );

            modules.push_back( md.getId() );
        }
    }

    return world;
}

} // namespace rofi::configuration
