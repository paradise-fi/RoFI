//
// Created by xvozarov on 10/15/18.
//

#ifndef ROBOTY_MATRIX_H
#define ROBOTY_MATRIX_H

#include <armadillo>

using Vector = arma::vec4;
using Matrix = arma::mat44;

inline const Matrix identity = arma::mat(4, 4, arma::fill::eye);
inline const Vector X = arma::vec4({1,0,0,0});
inline const Vector Y = arma::vec4({0,1,0,0});
inline const Vector Z = arma::vec4({0,0,1,0});

inline bool equals( const Matrix& a, const Matrix& b)
{
    return arma::approx_equal( a, b, "absdiff", 0.001);
}


inline Vector column(const Matrix &matrix, int col)
{
    return arma::vec4{matrix(0, col), matrix(1, col), matrix(2, col), matrix(3, col)};
}


inline Vector row(const Matrix &matrix, int row)
{
    return Vector{matrix(row, 0), matrix(row, 1), matrix(row, 2), matrix(row, 3)};
}

inline double distance(const Vector& a, const Vector& b)
{
    return std::round(
            sqrt((a(0) - b(0)) * (a(0) - b(0)) + (a(1) - b(1)) * (a(1) - b(1)) + (a(2) - b(2)) * (a(2) - b(2))) *
            1000) / 1000.0;
}

inline Matrix rotate(double r, const Vector &u)
{
    const int x = 0, y = 1, z = 2;
    Matrix rotate = identity;

    rotate(0,0) = cos(r) + u(x) * u(x) * (1 - cos(r));
    rotate(0,1) = u(x) * u(y) * (1 - cos(r)) - u(z) * sin(r);
    rotate(0,2) = u(x) * u(z) * (1 - cos(r)) + u(y) * sin(r);
    rotate(0,3) = 0;

    rotate(1,0) = u(x) * u(y) * (1 - cos(r)) + u(z) * sin(r);
    rotate(1,1) = cos(r) + u(y) * u(y) * (1 - cos(r));
    rotate(1,2) = u(y) * u(z) * (1 - cos(r)) - u(x) * sin(r);
    rotate(1,3) = 0;

    rotate(2,0) = u(z) * u(x) * (1 - cos(r)) - u(y) * sin(r);
    rotate(2,1) = u(z) * u(y) * (1 - cos(r)) + u(x) * sin(r);
    rotate(2,2) = cos(r) + u(z) * u(z) * (1 - cos(r));
    rotate(2,3) = 0;

    rotate(3) = {0,0,0,1};
    return rotate;
}

inline Matrix translate(const Vector &u)
{
    const int x = 0, y = 1, z = 2;
    Matrix translate = { {1, 0, 0, u(x)}, {0, 1, 0, u(y)}, {0, 0, 1, u(z)}, {0,0,0,1} };
    return translate;
}

inline Matrix inversion()
{
    Matrix inverse = { {-1,0,0,0}, {0,-1,0,0}, {0,0,-1,0}, {0,0,0,1} };
    return inverse;
}

inline Matrix flip()
{
    const int x = 0, y = 1, z = 2;
    Matrix flip = { {1, 0, 0, 0}, {0, -1, 0, 0}, {0, 0, -1, 0}, {0, 0, 0, 1} };
    return flip;
}


#endif //ROBOTY_MATRIX_H
