#pragma once

#include <iomanip>
#include <sstream>

#include "Matrix.h"

namespace rofi::configuration::matrices
{

inline std::string to_string( const Matrix & matrix )
{
    std::stringstream out;
    out << std::fixed << std::setprecision( 2 );

    for ( auto row = 0u; row < matrix.n_rows; row++ ) {
        for ( auto col = 0u; col < matrix.n_cols; col++ ) {
            out << std::setw( 7 ) << matrix.at( row, col );
        }
        out << '\n';
    }

    return std::move( out ).str();
}

} // namespace rofi::configuration::matrices
