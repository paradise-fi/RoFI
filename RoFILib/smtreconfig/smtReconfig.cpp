#include "smtReconfig.hpp"
#include "smt.hpp"
#include <fmt/format.h>

namespace rofi::smtr {

void collectVar( const SinCosAngle& a, std::vector< z3::expr >& out ) {
    out.insert( out.end(), { a.sin, a.sinhalf, a.cos, a.coshalf } );
}


void collectVar( const Shoe& s, std::vector< z3::expr >& out ) {
    out.insert( out.end(), { s.x, s.y, s.z, s.qa, s.qb, s.qc, s.qd } );
}

void collectVar( const Module& m, std::vector< z3::expr >& out ) {
    collectVar( m.alpha, out );
    collectVar( m.beta, out );
    collectVar( m.gamma, out );
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

SinCosAngle buildAngle( z3::context& c, std::string name ) {
    return {
        realVar( c, name + "_sin" ),
        realVar( c, name + "_sinhalf" ) ,
        realVar( c, name + "_cos" ),
        realVar( c, name + "_coshalf" )
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
            buildAngle( ctx, fmt::format( modulePrefix + "alpha", cfgId, moduleId ) ),
            buildAngle( ctx, fmt::format( modulePrefix + "beta", cfgId, moduleId ) ),
            buildAngle( ctx, fmt::format( modulePrefix + "gamma", cfgId, moduleId ) ),
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
        const auto& shoeM = cfg.modules[ m ].shoes[ ms ];
        const auto& shoeN = cfg.modules[ n ].shoes[ ns ];
        phi = phi &&
            ( square( shoeM.x - shoeN.x ) +
            square( shoeM.y - shoeN.y ) +
            square( shoeM.z - shoeN.z ) ) >= 1;

    }
    return phi;
}

z3::expr phiIsConnected( const SmtConfiguration& cfg ) {
    return cfg.context.bool_val( true ); // ToDo
}

z3::expr phiShoeConsistent( const SmtConfiguration& cfg ) {
    using namespace smt;
    z3::expr phi = cfg.context.bool_val( true );
    for ( const auto& module : cfg.modules ) {
        const auto a = module.shoes[ ShoeId::A ];
        const auto b = module.shoes[ ShoeId::B ];
        phi = phi &&
            b.x == a.x
                + 2 * module.alpha.cos * ( a.qa * a.qc + a.qb * a.qd )
                + 2 * module.alpha.sin * ( a.qa * a.qd - a.qb * a.qc )
            &&
            b.y == a.y
                + 2 * module.alpha.cos * ( a.qc * a.qd - a.qa * a.qb )
                + module.alpha.sin * ( -a.qa * a.qa + a.qb * a.qb - a.qc * a.qc + a.qd * a.qd )
            &&
            b.z == a.z
                + module.alpha.cos * ( a.qa * a.qa - a.qb * a.qb - a.qc * a.qc + a.qd * a.qd )
                - 2 * module.alpha.sin * (a.qa * a.qb + a.qc * a.qd );

        phi = phi &&
            b.qa ==
                 a.qa * module.alpha.coshalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qa * module.alpha.sinhalf * module.beta.coshalf * module.gamma.sinhalf
                + a.qb * module.alpha.coshalf * module.beta.coshalf * module.gamma.sinhalf
                - a.qb * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.sinhalf
                - a.qc * module.alpha.coshalf * module.beta.coshalf * module.gamma.coshalf
                - a.qc * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.coshalf
                + a.qd * module.alpha.coshalf * module.beta.sinhalf * module.gamma.coshalf
                - a.qd * module.alpha.sinhalf * module.beta.coshalf * module.gamma.coshalf
            &&
            b.qb ==
                -a.qa * module.alpha.coshalf * module.beta.coshalf * module.gamma.sinhalf
                + a.qa * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qb * module.alpha.coshalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qb * module.alpha.sinhalf * module.beta.coshalf * module.gamma.sinhalf
                - a.qc * module.alpha.coshalf * module.beta.sinhalf * module.gamma.coshalf
                + a.qc * module.alpha.sinhalf * module.beta.coshalf * module.gamma.coshalf
                - a.qd * module.alpha.coshalf * module.beta.coshalf * module.gamma.coshalf
                - a.qd * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.coshalf
            &&
            b.qc ==
                  a.qa * module.alpha.coshalf * module.beta.coshalf * module.gamma.coshalf
                + a.qa * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.coshalf
                + a.qb * module.alpha.coshalf * module.beta.sinhalf * module.gamma.coshalf
                - a.qb * module.alpha.sinhalf * module.beta.coshalf * module.gamma.coshalf
                + a.qc * module.alpha.coshalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qc * module.alpha.sinhalf * module.beta.coshalf * module.gamma.sinhalf
                - a.qd * module.alpha.coshalf * module.beta.coshalf * module.gamma.sinhalf
                + a.qd * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.sinhalf
            &&
            b.qd ==
                - a.qa * module.alpha.coshalf * module.beta.sinhalf * module.gamma.coshalf
                + a.qa * module.alpha.sinhalf * module.beta.coshalf * module.gamma.coshalf
                + a.qb * module.alpha.coshalf * module.beta.coshalf * module.gamma.coshalf
                + a.qb * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.coshalf
                + a.qc * module.alpha.coshalf * module.beta.coshalf * module.gamma.sinhalf
                - a.qc * module.alpha.sinhalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qd * module.alpha.coshalf * module.beta.sinhalf * module.gamma.sinhalf
                + a.qd * module.alpha.sinhalf * module.beta.coshalf * module.gamma.sinhalf
            ;
    }
    return phi;
}

z3::expr phiSinCos( const SinCosAngle& a ) {
    using namespace smt;
    return
        square( a.sin ) + square( a.cos ) == 1 &&
        a.sin == 2 * a.sinhalf * a.coshalf &&
        a.cos == 1 - 2 * square( a.sinhalf ) &&
        a.sin >= -1 && a.sin <= 1 &&
        a.cos >= -1 && a.cos <= 1 &&
        a.sinhalf >= -1 && a.sinhalf <= 1 &&
        a.coshalf >= -1 && a.coshalf <= 1;
}

z3::expr phiSinCos( const SmtConfiguration& cfg ) {
    z3::expr phi = cfg.context.bool_val( true );
    for ( const auto& module : cfg.modules ) {
        phi = phi &&
            phiSinCos( module.alpha ) && phiSinCos( module.beta ) && phiSinCos( module.gamma );
    }
    return phi;
}

} // namespace rofi::smtr