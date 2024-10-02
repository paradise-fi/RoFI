#pragma once

#include <z3++.h>
#include <vector>

namespace smt {

inline z3::expr square( const z3::expr& x ) {
    return x * x;
}

inline z3::expr implies( const z3::expr& x, const z3::expr& y ) {
    return !x || y;
}

z3::expr atMostOne( z3::context& ctx, const std::vector< z3::expr >& vars );

} // namespace smt
