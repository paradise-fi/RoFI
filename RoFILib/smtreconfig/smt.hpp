#pragma once

#include <z3++.h>

namespace smt {

inline z3::expr square( const z3::expr& x ) {
    return x * x;
}

} // namespace smt