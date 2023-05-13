#pragma once

#include <armadillo>

/**
 * Get ID of joint in this system transformed from RofiWorld.
 * @param rofiModuleId ID of Module
 * @param componentIdx Index of component in Module
 * @return
 */
int getJointId(int rofiModuleId, int componentIdx);

struct IntPairHash
{
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ (std::hash<T2>()(pair.second) << 1);
    }
};

/**
 * Checks whether a and b is close.
 * @param a First number
 * @param b Second number
 * @param epsilon Max difference. Default is 10^-6
 * @return True if absolute value of difference is less than epsilon, false otherwise
 */
bool almostEqual(double a, double b, double epsilon=1e-6);

/**
 * Project vector in another vector direction.
 * @param vector Vector to project
 * @param to Direction in which will be vector projected
 * @return Projected vector
 */
arma::vec3 projectVectorOntoAnother(const arma::vec3& vector, const arma::vec3& to);

/**
 * Generate matrix m which is projection of rows of input matrix in vector direction. For i-th row of result matrix m holds:
 * m[i] = projectVectorOntoAnother(matrix[i], to)
 * @param matrix Matrix to project
 * @param to Direction in which will be rows of vector projected
 * @return Matrix with projected rows
 */
arma::mat33 projectMatrixOntoVector(const arma::mat33& matrix, const arma::vec3& to);

/**
 * Generate matrix m which is projection of rows of input matrix to plane given by its norm vector. For i-th row of result matrix m holds:
 * m[i] * projectVectorOntoAnother(matrix[i], to) = 0
 * @param matrix Matrix to project
 * @param norm Norm vector of plane of projection
 * @return Matrix with projected rows
 */
arma::mat33 projectMatrixOntoPlane(const arma::mat33& matrix, const arma::vec3& norm);

/**
 * Compute vector v such that divisor x v = dividend.
 * @param dividend
 * @param divisor
 * @return Result vector v
 */
arma::vec3 crossDivision(const arma::vec3& dividend, const arma::vec3& divisor);

/**
 * Generate vector u such that u . vector = 0.
 * https://math.stackexchange.com/questions/137362/how-to-find-perpendicular-vector-to-another-vector
 * @param vector
 * @return Vector perpendicular to input vector
 */
arma::vec3 generatePerpendicularVector(const arma::vec3& vector);

/**
 * Generate matrix m such that for each i-th row of each matrix a holds:
 * (a @ m)[i] = a[i] x vector
 * @param vector
 * @return Result matrix m
 */
arma::mat33 getCrossProductMatrix(const arma::vec3& vector);

/**
 * Generate matrix m such that for each i-th row of each matrix a holds:
 * (a @ m)[i] = crossDivision(a[i], divisor) iff crossDivision does not throw
 * @param divisor
 * @return Result matrix m
 */
arma::mat33 getCrossDivisionMatrix(const arma::vec3& divisor);

/**
 * Check if u and v are parallel. Precision to ~1 thousands.
 * @param u
 * @param v
 * @return True if vectors are parallel, false otherwise.
 */
bool isParallel(const arma::vec3& u, const arma::vec3& v);

/**
 * Compute hash of arma vector.
 * @param v
 * @return
 */
size_t hashVec(const arma::vec3& v);