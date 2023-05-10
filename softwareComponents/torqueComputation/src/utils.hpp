#pragma once

#include <armadillo>

/**
 * Get ID of joint in this system transformed from RofiWorld.
 * @param rofiModuleId
 * @param componentIdx
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
 * @param a
 * @param b
 * @param epsilon
 * @return
 */
bool almostEqual(double a, double b, double epsilon=1e-6);

/**
 * Project vector in another vector direction.
 * @param vector
 * @param to
 * @return
 */
arma::vec3 projectVectorOntoAnother(const arma::vec3& vector, const arma::vec3& to);

/**
 * Project matrix rows in vector direction. Each row is then parallel with provided vector.
 * @param matrix
 * @param to
 * @return
 */
arma::mat33 projectMatrixOntoVector(const arma::mat33& matrix, const arma::vec3& to);

/**
 * Project matrix rows to plane given by its norm vector.
 * @param matrix
 * @param norm
 * @return
 */
arma::mat33 projectMatrixOntoPlane(const arma::mat33& matrix, const arma::vec3& norm);

/**
 * Compute vector v such that divisor x v = dividend.
 * @param dividend
 * @param divisor
 * @return
 */
arma::vec3 crossDivision(const arma::vec3& dividend, const arma::vec3& divisor);

/**
 * Generate vector u such that u . vector = 0.
 * https://math.stackexchange.com/questions/137362/how-to-find-perpendicular-vector-to-another-vector
 * @param vector
 * @return
 */
arma::vec3 generatePerpendicularVector(const arma::vec3& vector);

/**
 * Generate matrix m such that for each i-th row of each matrix a holds:
 * (a @ m)[i] = a[i] x vector
 * @param vector
 * @return
 */
arma::mat33 getCrossProductMatrix(const arma::vec3& vector);

/**
 * Generate matrix m such that for each i-th row of each matrix a holds:
 * (a @ m)[i] = cross_division(a[i], divisor) iff cross_division does not throw
 * @param divisor
 * @return
 */
arma::mat33 getCrossDivisionMatrix(const arma::vec3& divisor);

/**
 * Check if u and v are parallel. Precision to ~1 thousands.
 * @param u
 * @param v
 * @return
 */
bool isParallel(const arma::vec3& u, const arma::vec3& v);

/**
 * Compute hash of arma vector.
 * @param v
 * @return
 */
size_t hashVec(const arma::vec3& v);