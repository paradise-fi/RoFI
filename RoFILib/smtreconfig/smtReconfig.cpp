#include "smtReconfig.hpp"
#include "smt.hpp"
#include <fmt/format.h>

namespace rofi::smtr {

void collectVar( const Shoe& s, std::vector< z3::expr >& out ) {
    out.insert( out.end(), { s.x, s.y, s.z, s.qa, s.qb, s.qc, s.qd } );
}

void collectVar( const Module& m, std::vector< z3::expr >& out ) {
    out.insert( out.end(), { m.alpha, m.beta, m.gamma } );
    for ( const auto& shoe : m.shoes ) {
        collectVar( shoe, out );
    }
}

void collectVar( const SmtConfiguration& c, std::vector< z3::expr >& out ) {
    for ( const auto& module : c.modules ) {
        collectVar( module, out );
    }
    for ( const auto& m : c.connections ) {
        for ( const auto& ms : m ) {
            for ( const auto& n : ms ) {
                for ( const auto& ns : n ) {
                    for ( const auto& var : ns ) {
                        out.push_back( var );
                    }
                }
            }
        }
    }
}

std::string toString( ShoeId s ) {
    std::string names[] = { "A", "B" };
    return names[ s ];
}

std::string toString( ConnectorId c ) {
    std::string names[] = { "+X", "-X", "-Z" };
    return names[ c ];
}

std::string toString( Orientation o ) {
    std::string names[] = { "N", "W", "S", "E" };
    return names[ o ];
}

template < typename... Args >
z3::expr realVar( z3::context& c, const std::string& format, Args... args ) {
    return c.real_const( fmt::format( format, args... ).c_str() );
}

template < typename... Args >
z3::expr boolVar( z3::context& c, const std::string& format, Args... args ) {
    return c.bool_const( fmt::format( format, args... ).c_str() );
}

Shoe buildShoe( z3::context& c, std::string shoeName, int moduleId, int cfgId ) {
    const std::string shoePrefix = "cfg{}_m{}_s" + shoeName + "_";
    return {
        realVar( c, shoePrefix + "x", cfgId, moduleId ),
        realVar( c, shoePrefix + "y", cfgId, moduleId ),
        realVar( c, shoePrefix + "z", cfgId, moduleId ),
        realVar( c, shoePrefix + "qa", cfgId, moduleId ),
        realVar( c, shoePrefix + "qb", cfgId, moduleId ),
        realVar( c, shoePrefix + "qc", cfgId, moduleId ),
        realVar( c, shoePrefix + "qd", cfgId, moduleId )
    };
}

std::vector< std::tuple< int, ShoeId, int, ShoeId > > allShoePairs( int count ) {
    std::vector< std::tuple< int, ShoeId, int, ShoeId > > ret;

    for ( int m = 0; m < count; m++ ) {
        for ( auto ms : { ShoeId::A, ShoeId::B } ) {
            for ( int n = m + 1; n < count; n++ ) {
                for ( auto ns : { ShoeId::A, ShoeId::B } ) {
                    ret.push_back( { m, ms, n, ns } );
                }
            }
        }
    }
    return ret;
}

auto allShoeConnections() ->
    std::vector< std::tuple< ConnectorId, ConnectorId, Orientation > >
{
    std::vector< std::tuple< ConnectorId, ConnectorId, Orientation > > ret;
    for ( ConnectorId mc : { XPlus, XMinus, ZMinus } ) {
        for ( ConnectorId nc : { XPlus, XMinus, ZMinus } ) {
            for ( Orientation o : { North, West, South, East} ) {
                ret.push_back( { mc, nc, o } );
            }
        }
    }
    return ret;
}

SmtConfiguration buildConfiguration( z3::context& ctx,
    const Configuration& cfg, int cfgId )
{
    SmtConfiguration smtCfg{ {}, {}, ctx };

    // Add modules
    const std::string modulePrefix = "cfg{}_m{}_";
    std::vector moduleIds = cfg.getIDs();
    std::sort( moduleIds.begin(), moduleIds.end() );
    for( const auto& moduleId : moduleIds ) {
        Module m {
            realVar( ctx, modulePrefix + "alpha", cfgId, moduleId ),
            realVar( ctx, modulePrefix + "beta", cfgId, moduleId ),
            realVar( ctx, modulePrefix + "gamma", cfgId, moduleId ),
            {
                buildShoe( ctx, "A", cfgId, moduleId ),
                buildShoe( ctx, "B", cfgId, moduleId )
            }
        };
        smtCfg.modules.push_back( m );
    };

    // Add connections
    for ( auto [ m, ms, n, ns ] : allShoePairs( moduleIds.size() ) ) {
        if ( smtCfg.connections.size() <= m )
            smtCfg.connections.push_back( {} );
        assert( smtCfg.connections.size() == m + 1 );
        auto& cell = smtCfg.connections[ m ][ ms ];
        if ( cell.size() <= n - m - 1 )
            cell.push_back( {} );
        assert( cell.size() == n - m );

        std::vector< z3::expr >& conn = cell[ n - m - 1 ][ ns ];
        for ( auto [ mc, nc, o ] : allShoeConnections() ) {
            conn.push_back(
                boolVar( ctx,
                    "cfg{}_c_{}{}{}_{}{}{}_{}",
                    cfgId,
                    moduleIds[ m ], toString( ms ), toString( mc ),
                    moduleIds[ n ], toString( ns ), toString( nc ),
                    toString( o ) )
                );
        }
    }

    return smtCfg;
}

z3::expr phiValid( const SmtConfiguration& cfg ) {
    return phiConsistent( cfg ) && phiNoIntersect( cfg ) && phiIsConnected( cfg );
}

z3::expr phiConsistent( const SmtConfiguration& cfg ) {
    return cfg.context.bool_val( true ); // ToDo
}

z3::expr phiNoIntersect( const SmtConfiguration& cfg ) {
    using namespace smt;
    z3::expr phi = cfg.context.bool_val( true );
    for ( auto [ m, ms, n, ns ] : allShoePairs( cfg.modules.size() ) ) {
        auto shoeM = cfg.modules[ m ].shoes[ ms ];
        auto shoeN = cfg.modules[ n ].shoes[ ns ];
        phi = phi &&
            ( square( shoeM.x - shoeN.x ) +
            square( shoeM.y - shoeN.y ) +
            square( shoeM.z - shoeN.z ) ) >= cfg.context.real_val( 1 );

    }
    return phi;
}

z3::expr phiIsConnected( const SmtConfiguration& cfg ) {
    return cfg.context.bool_val( true ); // ToDo
}

} // namespace rofi::smtr