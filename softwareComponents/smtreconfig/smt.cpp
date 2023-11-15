#include "smt.hpp"

namespace smt {

z3::expr atMostOne( z3::context& ctx, const std::vector< z3::expr >& vars ) {
    z3::expr none = ctx.bool_val( true );
    for ( const auto& var : vars )
        none = none && !var;
    z3::expr one = ctx.bool_val( true );
    for ( int i = 0; i < vars.size(); i++ ) {
        for ( int j = i + 1; j < vars.size(); j++ ) {
            one = one && (!vars[ i ] || !vars[ j ]);
        }
    }
    return none || one;
}

} // namespace smt
