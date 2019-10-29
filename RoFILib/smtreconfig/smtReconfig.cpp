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

SmtConfiguration buildConfiguration( Context& ctx,
    const Configuration& cfg, int cfgId )
{
    SmtConfiguration smtCfg{ {}, {} };

    // Add modules
    const std::string modulePrefix = "cfg{}_m{}_";
    std::vector moduleIds = cfg.getIDs();
    std::sort( moduleIds.begin(), moduleIds.end() );
    for( const auto& moduleId : moduleIds ) {
        Module m {
            buildAngle( ctx.ctx, fmt::format( modulePrefix + "alpha", cfgId, moduleId ) ),
            buildAngle( ctx.ctx, fmt::format( modulePrefix + "beta", cfgId, moduleId ) ),
            buildAngle( ctx.ctx, fmt::format( modulePrefix + "gamma", cfgId, moduleId ) ),
            {
                buildShoe( ctx.ctx, "A", cfgId, moduleId ),
                buildShoe( ctx.ctx, "B", cfgId, moduleId )
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
                boolVar( ctx.ctx,
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

z3::expr phiValid( Context& ctx, const SmtConfiguration& cfg ) {
    return phiConsistent( ctx, cfg ) && phiNoIntersect( ctx, cfg )
        && phiIsConnected( ctx, cfg );
}

z3::expr phiConsistent( Context& ctx, const SmtConfiguration& cfg ) {
    return phiShoeConsistent( ctx, cfg ) && phiConnectorConsistent( ctx, cfg );
}

z3::expr phiNoIntersect( Context& ctx, const SmtConfiguration& cfg ) {
    using namespace smt;
    z3::expr phi = ctx.ctx.bool_val( true );
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

z3::expr phiIsConnected( Context& ctx, const SmtConfiguration& cfg ) {
    return ctx.ctx.bool_val( true ); // ToDo
}

z3::expr phiShoeConsistent( Context& ctx, const SmtConfiguration& cfg ) {
    using namespace smt;
    z3::expr phi = ctx.ctx.bool_val( true );
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

z3::expr phiSinCos( Context&, const SinCosAngle& a ) {
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

z3::expr phiSinCos( Context& ctx, const SmtConfiguration& cfg ) {
    z3::expr phi = ctx.ctx.bool_val( true );
    for ( const auto& module : cfg.modules ) {
        phi = phi &&
            phiSinCos( ctx, module.alpha ) &&
            phiSinCos( ctx, module.beta ) &&
            phiSinCos( ctx, module.gamma );
    }
    return phi;
}

z3::expr otherShoeOrientation( Context& ctx, const Shoe& a, const Shoe& b,
                               ConnectorId ac, ConnectorId bc, Orientation o )
{
    // This function was generated by the Jupyter notebook using Sympy
    if ( ac == XPlus && bc == XPlus && o == North ){
        return b.qa == - a.qb && b.qb == a.qa && b.qc == a.qd && b.qd == - a.qc;
    }
    if ( ac == XPlus && bc == XPlus && o == West ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd );
    }
    if ( ac == XPlus && bc == XPlus && o == South ){
        return b.qa == a.qa && b.qb == a.qb && b.qc == a.qc && b.qd == a.qd;
    }
    if ( ac == XPlus && bc == XPlus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd );
    }
    if ( ac == XPlus && bc == XMinus && o == North ){
        return b.qa == a.qd && b.qb == - a.qc && b.qc == a.qb && b.qd == - a.qa;
    }
    if ( ac == XPlus && bc == XMinus && o == West ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qb ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb );
    }
    if ( ac == XPlus && bc == XMinus && o == South ){
        return b.qa == a.qc && b.qb == a.qd && b.qc == - a.qa && b.qd == - a.qb;
    }
    if ( ac == XPlus && bc == XMinus && o == East ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd ) && b.qb == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qb );
    }
    if ( ac == XPlus && bc == ZMinus && o == North ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qc ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qb + a.qd ) && b.qc == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd );
    }
    if ( ac == XPlus && bc == ZMinus && o == West ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qb == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qc == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qd == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == XPlus && bc == ZMinus && o == South ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qb + a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qc );
    }
    if ( ac == XPlus && bc == ZMinus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qb == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qc == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qd == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == XMinus && bc == XPlus && o == North ){
        return b.qa == - a.qd && b.qb == a.qc && b.qc == - a.qb && b.qd == a.qa;
    }
    if ( ac == XMinus && bc == XPlus && o == West ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc - a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qc == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb );
    }
    if ( ac == XMinus && bc == XPlus && o == South ){
        return b.qa == a.qc && b.qb == a.qd && b.qc == - a.qa && b.qd == - a.qb;
    }
    if ( ac == XMinus && bc == XPlus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc - a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb );
    }
    if ( ac == XMinus && bc == XMinus && o == North ){
        return b.qa == - a.qb && b.qb == a.qa && b.qc == a.qd && b.qd == - a.qc;
    }
    if ( ac == XMinus && bc == XMinus && o == West ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd );
    }
    if ( ac == XMinus && bc == XMinus && o == South ){
        return b.qa == - a.qa && b.qb == - a.qb && b.qc == - a.qc && b.qd == - a.qd;
    }
    if ( ac == XMinus && bc == XMinus && o == East ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qb ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qb ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qc + a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qc + a.qd );
    }
    if ( ac == XMinus && bc == ZMinus && o == North ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qb == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qc ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb - a.qd );
    }
    if ( ac == XMinus && bc == ZMinus && o == West ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qb == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qc == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qd == ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == XMinus && bc == ZMinus && o == South ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb - a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qc ) && b.qc == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc );
    }
    if ( ac == XMinus && bc == ZMinus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qb == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qc == ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qd == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == ZMinus && bc == XPlus && o == North ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qc ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qb + a.qd ) && b.qc == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd );
    }
    if ( ac == ZMinus && bc == XPlus && o == West ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qb == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qc == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qd == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == ZMinus && bc == XPlus && o == South ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qb + a.qd ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qc ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qd == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc );
    }
    if ( ac == ZMinus && bc == XPlus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qb == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qc == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qd == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == ZMinus && bc == XMinus && o == North ){
        return b.qa == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qb == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qc ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb - a.qd );
    }
    if ( ac == ZMinus && bc == XMinus && o == West ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qb == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qc == ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qd == - ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == ZMinus && bc == XMinus && o == South ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qd ) && b.qb == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qc ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb - a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qc );
    }
    if ( ac == ZMinus && bc == XMinus && o == East ){
        return b.qa == - ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qb == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb + ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd && b.qc == ctx.ctx.real_val(1, 2) * a.qa - ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc + ctx.ctx.real_val(1, 2) * a.qd && b.qd == ctx.ctx.real_val(1, 2) * a.qa + ctx.ctx.real_val(1, 2) * a.qb - ctx.ctx.real_val(1, 2) * a.qc - ctx.ctx.real_val(1, 2) * a.qd;
    }
    if ( ac == ZMinus && bc == ZMinus && o == North ){
        return b.qa == a.qb && b.qb == - a.qa && b.qc == - a.qd && b.qd == a.qc;
    }
    if ( ac == ZMinus && bc == ZMinus && o == West ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qc ) && b.qb == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qa + a.qd ) && b.qc == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( -a.qb + a.qc );
    }
    if ( ac == ZMinus && bc == ZMinus && o == South ){
        return b.qa == a.qc && b.qb == a.qd && b.qc == - a.qa && b.qd == - a.qb;
    }
    if ( ac == ZMinus && bc == ZMinus && o == East ){
        return b.qa == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb - a.qc ) && b.qb == - ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa + a.qd ) && b.qc == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qa - a.qd ) && b.qd == ctx.ctx.real_val(1, 2) * ctx.sqrt2 * ( a.qb + a.qc );
    }
    assert(false);
    __builtin_unreachable();
}

z3::expr otherShoePosition( Context& ctx, const Shoe& a, const Shoe& b,
                            ConnectorId c ) {
    // This function was generated by the Jupyter notebook using Sympy
    if ( c == XPlus) {
        return b.x == - 2 * a.qc * a.qc - 2 * a.qd * a.qd + a.x + 1 &&
               b.y == 2 * a.qa * a.qd + 2 * a.qb * a.qc + a.y &&
               b.z == - 2 * a.qa * a.qc + 2 * a.qb * a.qd + a.z;
    }
    if ( c == XMinus) {
        return b.x == 2 * a.qc * a.qc + 2 * a.qd * a.qd + a.x - 1 &&
               b.y == - 2 * a.qa * a.qd - 2 * a.qb * a.qc + a.y &&
               b.z == 2 * a.qa * a.qc - 2 * a.qb * a.qd + a.z;
    }
    if ( c == ZMinus) {
        return b.x == - 2 * a.qa * a.qc - 2 * a.qb * a.qd + a.x &&
               b.y == 2 * a.qa * a.qb - 2 * a.qc * a.qd + a.y &&
               b.z == 2 * a.qb * a.qb + 2 * a.qc * a.qc + a.z - 1;
    }
    assert(false);
    __builtin_unreachable();
}

z3::expr phiConnectorConsistent( Context& ctx, const SmtConfiguration& cfg ) {
    z3::expr phi = ctx.ctx.bool_val( true );

    for ( auto [ m, ms, n, ns ] : allShoePairs( cfg.modules.size() ) ) {
        for ( auto [ mc, nc, o ] : allShoeConnections() ) {
            auto shoeA = cfg.modules[ m ].shoes[ ms ];
            auto shoeB = cfg.modules[ n ].shoes[ ns ];
            phi = phi && smt::implies( cfg.connection( m, ms, mc, n, ns, nc, o ),
                otherShoePosition( ctx, shoeA, shoeB, mc ) &&
                otherShoeOrientation( ctx, shoeA, shoeB, mc, nc, o )
            );
        }
    }

    return phi;
}

z3::expr symSin( Context& ctx, double val ) {
    assert( !std::isnan( val ) );
    if ( val < 0 )
        return symSin( ctx, val + 2 * M_PI );
    if ( val > 2 * M_PI )
        return symSin( ctx, val - 2 * M_PI );

    if ( val <= M_PI / 2) {
        if ( val == 0 )
            return ctx.ctx.real_val( 0 );
        if ( val ==  M_PI / 6 )
            return ctx.ctx.real_val( 1, 2 );
        if ( val == M_PI / 4 )
            return ctx.sqrt2 / 2;
        if ( val == M_PI / 3 )
            return ctx.sqrt3 / 2;
        if ( val == M_PI / 2 )
            return ctx.ctx.real_val( 1 );
        return ctx.ctx.real_val( std::to_string( sin( val ) ).c_str() );
    }
    if ( val <= M_PI )
        return symSin( ctx, M_PI - val );
    if ( val <= 3 * M_PI / 2 )
        return - symSin( ctx, val - M_PI );
    if ( val <= 2 * M_PI )
        return - symSin( ctx, 2 * M_PI - val );
    return ctx.ctx.real_val( std::to_string( sin( val ) ).c_str() );
}

z3::expr symCos( Context& ctx, double val ) {
    return symSin( ctx, val + M_PI / 2 );
}

z3::expr phiEqual( Context& ctx, const SinCosAngle& a, double val ) {
    return a.sin == symSin( ctx, val ) && a.sinhalf == symSin( ctx, val / 2 ) &&
           a.cos == symCos( ctx, val ) && a.coshalf == symCos( ctx, val / 2 );
}

z3::expr phiEqual( Context& ctx, const SinCosAngle& a, const SinCosAngle& b ) {
    return a.sin == b.sin && a.sinhalf == b.sinhalf &&
           a.cos == b.cos && a.coshalf == b.coshalf;
}

double degToRad( double d ) {
    return M_PI * d / 180;
}

z3::expr phiEqual( Context& ctx, const SmtConfiguration& smtCfg,
    Configuration& cfg )
{
    z3::expr phi = ctx.ctx.bool_val( true );

    std::vector moduleIds = cfg.getIDs();
    std::sort( moduleIds.begin(), moduleIds.end() );
    for ( int i = 0; i != moduleIds.size(); i++ ) {
        const auto& module = cfg.getModule( moduleIds[ i ] );
        const auto& smtModule = smtCfg.modules[ i ];
        phi = phi &&
            phiEqual( ctx, smtModule.alpha, degToRad( module.getJoint( Alpha ) ) ) &&
            phiEqual( ctx, smtModule.beta, degToRad( module.getJoint( Beta ) ) ) &&
            phiEqual( ctx, smtModule.gamma, degToRad( module.getJoint( Gamma ) ) );
    }

    auto moduleIdx = [&]( int id ) -> int {
        auto it = std::lower_bound( moduleIds.begin(), moduleIds.end(), id );
        assert( it != moduleIds.end() && *it == id );
        return std::distance( moduleIds.begin(), it );
    };

    for ( auto [ m, ms, n, ns ] : allShoePairs( moduleIds.size() ) ) {
        for ( auto [ mc, nc, o ] : allShoeConnections() ) {
            Edge e(moduleIds[ m ], ms, mc, o, nc, ns, n );
            const auto& connection = smtCfg.connection( m, ms, mc, n, ns, nc, o );
            if ( cfg.findEdge( e ) )
                phi = phi && connection;
            else
                phi = phi && !connection;
        }
    }

    return phi;
}

z3::expr phiRootModule( Context& ctx, const SmtConfiguration& cfg,
                        int moduleIdx )
{
    const auto& shoe = cfg.modules[ 0 ].shoes[ A ];
    return shoe.x == 0 && shoe.y == 0 && shoe.z == 0 &&
        shoe.qa == 1 && shoe.qb == 0 && shoe.qc == 0 && shoe.qd;
}

z3::expr phiEqualJoints( Context& ctx, const SmtConfiguration& a,
    const SmtConfiguration& b )
{
    z3::expr phi = ctx.ctx.bool_val( true );

    for ( int i = 0; i != a.modules.size(); i++ ) {
        const auto& moduleA = a.modules[ i ];
        const auto& moduleB = b.modules[ i ];
        phi = phi &&
            phiEqual( ctx, moduleA.alpha, moduleB.alpha ) &&
            phiEqual( ctx, moduleA.beta, moduleB.beta ) &&
            phiEqual( ctx, moduleA.gamma, moduleB.gamma );
    }
    return phi;
}

} // namespace rofi::smtr