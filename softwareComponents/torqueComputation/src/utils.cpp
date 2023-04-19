#include "utils.hpp"

#include <atoms/units.hpp>

using namespace arma;

bool almostEqual(double a, double b, double epsilon/*=1e-6*/) {
    return std::abs(a - b) < epsilon;
}

int getJointId(int rofiModuleId, int componentIdx) {
    return rofiModuleId * 100 + componentIdx;
}

vec3 projectVectorOntoAnother(const vec3& vector, const vec3& to) {
    vec toNormalized = normalise(to);
    return dot(vector, toNormalized) * toNormalized;
}

mat33 projectMatrixOntoVector(const mat33& matrix, const vec3& to) {
    vec normNormalized = normalise(to);
    return matrix * (normNormalized * normNormalized.t());
}

mat33 projectMatrixOntoPlane(const mat33& matrix, const vec3& norm) {
    vec normNormalized = normalise(norm);
    return matrix * (eye(3, 3) - normNormalized * normNormalized.t());
}

vec3 crossDivision(const vec3& dividend, const vec3& divisor) {
    if (!almostEqual(dot(dividend, divisor), 0.0)) {
        throw std::logic_error("vectors are not perpendicular");
    }
    double lengthSquared = dot(divisor, divisor);
    if (lengthSquared == 0) {
        throw std::logic_error("division by zero");
    }
    return cross(dividend, divisor) / lengthSquared;
}

vec3 generatePerpendicularVector(const vec3& vector) {
    if(almostEqual(norm(vector), 0)) {
        throw std::logic_error("vector is 0");
    }
    return {
            copysign(vector[2], vector[0]),
            copysign(vector[2], vector[1]),
            - (copysign(vector[0], vector[2]) + copysign(vector[1], vector[2]))
    };
}

mat33 getCrossProductMatrix(const vec3& vector) {
    mat result = eye(vector.n_elem, vector.n_elem);

    for (uword i = 0; i < vector.n_elem; ++i) {
        result.row(i) = cross(result.row(i), vector.t());
    }

    return result;
}

mat33 getCrossDivisionMatrix(const vec3& divisor) {
    double lengthSquared = dot(divisor, divisor);
    if (lengthSquared == 0) {
        throw std::logic_error("division by zero");
    }
    mat divisorMatrix = getCrossProductMatrix(divisor);
    return divisorMatrix / lengthSquared;
}

mat33 rotate(double xDeg, double yDeg, double zDeg) {
    double alpha = Angle::deg(zDeg).rad();
    double beta = Angle::deg(yDeg).rad();
    double gamma = Angle::deg(xDeg).rad();

    double alphaSin = sin(alpha);
    double alphaCos = cos(alpha);

    double betaSin = sin(beta);
    double betaCos = cos(beta);

    double gammaSin = sin(gamma);
    double gammaCos = cos(gamma);

    mat rotationMatrix = {
            {
                alphaCos * betaCos,
                alphaCos * betaSin * gammaSin - alphaSin * gammaCos,
                alphaCos * betaSin * gammaCos + alphaSin * gammaSin
            },
            {
                alphaSin * betaCos,
                alphaSin * betaSin * gammaSin + alphaCos * gammaCos,
                alphaSin * betaSin * gammaCos - alphaCos * gammaSin
            },
            {
                -betaSin,
                betaCos * gammaSin,
                betaCos * gammaCos
            }
    };

    return rotationMatrix;
}

bool isParallel(const vec3& u, const vec3& v) {
    double uLength = norm(u);
    if (std::abs(uLength) <= 1e-9) {
        throw std::logic_error("First vector is zero.");
    }
    double vLength = norm(v);
    if (std::abs(vLength) <= 1e-9) {
        throw std::logic_error("Second vector is zero.");
    }
    return almostEqual(std::abs(dot(u, v) / (uLength * vLength)), 1);
}

size_t hashVec(const vec3& v) {
    size_t result = 0;
    for (double val : v) {
        result ^= std::hash<double>()(val);
    }
    return result;
}