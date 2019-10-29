#pragma once

#include <z3++.h>

namespace smt {

inline z3::expr square( const z3::expr& x ) {
    return x * x;
}

inline z3::expr implies( const z3::expr& x, const z3::expr& y ) {
    return !x || y;
}

} // namespace smt