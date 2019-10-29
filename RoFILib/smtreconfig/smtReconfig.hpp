#pragma once
#include <z3++.h>
#include <Configuration.h>
#include <array>

#include "mixins.hpp"

namespace rofi::smtr {

using ModuleIdx = int;

struct Shoe {
    z3::expr x, y, z; // Shoe center position
    z3::expr qa, qb, qc, qd; // Shoe orientation
};

struct SinCosAngle {
    z3::expr sin, sinhalf, cos, coshalf;
};

struct Module {
    SinCosAngle alpha, beta, gamma; // Joint position
    std::array< Shoe, 2 > shoes; // Shoes
};

struct SmtConfiguration {
    template < typename T > using PerModule = std::vector< T >;
    template < typename T > using PerShoe = std::array< T, 2 >;
    template < typename T > using PerModuleShoe = PerModule< PerShoe < T > >;

    PerModuleShoe< PerModuleShoe < std::vector< z3::expr > > > connections;
        // All possible connections between shoe connectors
    std::vector< Module > modules; // All modules in the configuration
    z3::context& context; // Z3 context for the variables

    z3::expr connection( ModuleIdx m, ShoeId ms, ConnectorId mc,
        ModuleIdx n, ShoeId ns, ConnectorId nc, Orientation o )
    {
        assert( m != n );
        if ( m > n ) {
            return connection( n, ns, nc, m, ms, mc, o );
        }
        assert( connections.size() > m );
        assert( connections[ m ][ ms ].size() > n - m - 1 );
        assert( connections[ m ][ ms ][ n - m - 1 ][ ns ].size() == 36 );
        return connections[ m ][ ms ][ n - m - 1 ][ ns ][ 3 * 4 * mc  + 4 * nc + o ];
    }
};

void collectVar( const SinCosAngle& a, std::vector< z3::expr >& out );
void collectVar( const Shoe& s, std::vector< z3::expr >& out );
void collectVar( const Module& m, std::vector< z3::expr >& out );
void collectVar( const SmtConfiguration& c, std::vector< z3::expr >& out );

template < typename T >
std::vector< z3::expr > collectVar( const T& o ) {
    std::vector< z3::expr > ret;
    collectVar( o, ret );
    return ret;
}

SmtConfiguration buildConfiguration( z3::context& ctx,
    const Configuration& cfg, int cfgId );

z3::expr phiValid( const SmtConfiguration& cfg );
z3::expr phiConsistent( const SmtConfiguration& cfg );
z3::expr phiNoIntersect( const SmtConfiguration& cfg );
z3::expr phiIsConnected( const SmtConfiguration& cfg );
z3::expr phiShoeConsistent( const SmtConfiguration& cfg );
z3::expr phiSinCos( const SmtConfiguration& cfg );

} // rofi::smtr