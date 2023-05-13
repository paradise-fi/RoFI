#include <catch2/catch.hpp>
#include <armadillo>

#include "../src/utils.hpp"

using namespace arma;

const double TOLERANCE = 1e-6;
const std::string INVARIANT_FAILED = "Function invariant invalid!";

TEST_CASE("Project vector onto another - zero vector", "[projectVectorOntoAnother]") {
    vec vector = zeros<vec>(3);
    vec to_vector = {1, 1, 1};
    vec expected = zeros<vec>(3);

    vec result = projectVectorOntoAnother(vector, to_vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
}

TEST_CASE("Project vector onto another - same direction, unchanged", "[projectVectorOntoAnother]") {
    vec vector = {2, 2, 2};
    vec to_vector = {1, 1, 1};
    vec expected = {2, 2, 2};

    vec result = projectVectorOntoAnother(vector, to_vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
}

TEST_CASE("Project vector onto another - opposite direction, unchanged", "[projectVectorOntoAnother]") {
    vec vector = {-2, -2, -2};
    vec to_vector = {1, 1, 1};
    vec expected = {-2, -2, -2};

    vec result = projectVectorOntoAnother(vector, to_vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
}

TEST_CASE("Project vector onto another - general case", "[projectVectorOntoAnother]") {
    vec vector = {7, 5, -1};
    // (5, -4, 2 * sqrt(2)) -> length 7
    vec to_vector = {5, -4, 2.828427125};
    vec expected = {1.241997232, -0.993597784, 0.702579731};

    vec result = projectVectorOntoAnother(vector, to_vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
}

TEST_CASE("Project matrix onto vector - identity to y", "[projectMatrixOntoVector]") {
    mat matrix = eye(3, 3);
    vec to = {0, 1, 0};

    mat expected = {
        {0., 0., 0.},
        {0., 1., 0.},
        {0., 0., 0.}
    };

    mat result = projectMatrixOntoVector(matrix, to);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    
    for (int i = 0; i < 3; i++) {
        vec result_invariant = result.row(i).t();

        vec expected_invariant = projectVectorOntoAnother(matrix.row(i).t(), to);

        CAPTURE(result_invariant);
        CAPTURE(INVARIANT_FAILED);
        REQUIRE(approx_equal(expected_invariant, result_invariant, "absdiff", TOLERANCE));
    }
}

TEST_CASE("Project matrix onto vector - identity to general", "[projectMatrixOntoVector]") {
    mat matrix = eye(3, 3);
    vec to = {1, 2, 3};
    mat expected = {
        {0.071429, 0.142857, 0.214286},
        {0.142857, 0.285714, 0.428571},
        {0.214286, 0.428571, 0.642857}
    };

    mat result = projectMatrixOntoVector(matrix, to);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    
    for (int i = 0; i < 3; i++) {
        vec result_invariant = result.row(i).t();

        vec expected_invariant = projectVectorOntoAnother(matrix.row(i).t(), to);

        CAPTURE(result_invariant);
        CAPTURE(INVARIANT_FAILED);
        REQUIRE(approx_equal(expected_invariant, result_invariant, "absdiff", TOLERANCE));
    }
}

TEST_CASE("Project matrix onto vector - general independent matrix to general" "[projectMatrixOntoVector]") {
    mat matrix = {
        {-1, 5, 9},
        {2, 7, -6},
        {4, 3, -8}
    };
    vec to = {1, 2, 3};
    mat expected = {
        {2.571429, 5.142857, 7.714286},
        {-0.142857, -0.285714, -0.428571},
        {-1., -2., -3.}
    };

    mat result = projectMatrixOntoVector(matrix, to);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    for (int i = 0; i < 3; i++) {
        vec result_invariant = result.row(i).t();

        vec expected_invariant = projectVectorOntoAnother(matrix.row(i).t(), to);

        CAPTURE(result_invariant);
        CAPTURE(INVARIANT_FAILED);
        REQUIRE(approx_equal(expected_invariant, result_invariant, "absdiff", TOLERANCE));
    }
}


TEST_CASE("Project matrix onto plane - identity to y", "[projectMatrixOntoPlane]") {
    mat matrix = eye<mat>(3, 3);
    vec norm = {0, 1, 0};
    mat expected = {
            {1, 0, 0},
            {0, 0, 0},
            {0, 0, 1}
    };

    mat result = projectMatrixOntoPlane(matrix, norm);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    for (int i = 0; i < 3; i++) {
        vec row = result.row(i).t();
        CAPTURE(row);
        REQUIRE(almostEqual(dot(row, norm), 0.0));
    }
}

TEST_CASE("Project matrix onto plane: identity to general", "[projectMatrixOntoPlane]") {
    mat matrix = eye<mat>(3, 3);
    vec norm = {1, 2, 3};
    mat expected = {
            {0.928571, -0.142857, -0.214286},
            {-0.142857, 0.714286, -0.428571},
            {-0.214286, -0.428571, 0.357143}
    };

    mat result = projectMatrixOntoPlane(matrix, norm);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    for (int i = 0; i < 3; i++) {
        vec row = result.row(i).t();
        CAPTURE(row);
        REQUIRE(almostEqual(dot(row, norm), 0.0));
    }
}

TEST_CASE("Project matrix onto plane: general independent matrix to general", "[projectMatrixOntoPlane]") {
    mat matrix = {
            {-1, 5, 9},
            {2, 7, -6},
            {4, 3, -8}
    };
    vec norm = {1, 2, 3};
    mat expected = {
            {-3.571429, -0.142857, 1.285714},
            {2.142857, 7.285714, -5.571429},
            {5., 5., -5.}
    };

    mat result = projectMatrixOntoPlane(matrix, norm);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    for (int i = 0; i < 3; i++) {
        vec row = result.row(i).t();
        CAPTURE(row);
        REQUIRE(almostEqual(dot(row, norm), 0.0));
    }
}

TEST_CASE("Project matrix onto plane: general dependent matrix to general", "[projectMatrixOntoPlane]") {
    mat matrix = {
            {1, 3, 2},
            {2, 6, 4},
            {4, 12, 8}
    };
    vec norm = {1, 2, 3};
    mat expected = {
            {0.071429, 1.142857, -0.785714},
            {0.142857, 2.285714, -1.571429},
            {0.285714, 4.571429, -3.142857}
    };

    mat result = projectMatrixOntoPlane(matrix, norm);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    for (int i = 0; i < 3; i++) {
        vec row = result.row(i).t();
        CAPTURE(row);
        REQUIRE(almostEqual(dot(row, norm), 0.0));
    }
}

TEST_CASE("Cross division: not perpendicular vectors raises assertion", "[crossDivision]") {
    vec dividend {0, 0, 1};
    vec divisor {1, 1, 1};

    REQUIRE_THROWS_AS(crossDivision(dividend, divisor), std::logic_error);
}

TEST_CASE("Cross division: zero divisor raises zero division", "[crossDivision]") {
    vec dividend {0, 0, 1};
    vec divisor {0, 0, 0};

    REQUIRE_THROWS_AS(crossDivision(dividend, divisor), std::logic_error);
}

TEST_CASE("Cross division: standard vector", "[crossDivision]") {
    vec dividend {0, 0, 1};
    vec divisor {1, 0, 0};
    vec expected {0, 1, 0};

    vec result = crossDivision(dividend, divisor);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    CAPTURE(INVARIANT_FAILED);
    REQUIRE(approx_equal(dividend, cross(divisor, result), "absdiff", TOLERANCE));
}

TEST_CASE("Cross division: general vector", "[crossDivision]") {
    vec dividend {1, 2, 3};
    vec divisor {2, -4, 2};
    vec expected {0.666666667, 0.166666667, -0.333333333};

    vec result = crossDivision(dividend, divisor);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    CAPTURE(INVARIANT_FAILED);
    REQUIRE(approx_equal(dividend, cross(divisor, result), "absdiff", TOLERANCE));
}

TEST_CASE("Generate perpendicular vector: zero vector raises", "[generatePerpendicularVector]") {
    vec zero_vector = zeros(3);

    REQUIRE_THROWS_AS(generatePerpendicularVector(zero_vector), std::logic_error);
}

TEST_CASE("Generate perpendicular vector: some coordinates are zero invariant holds", "[generatePerpendicularVector]") {
    for (auto vector : {
        vec{0, 0, 1},
        vec{0, 1, 0},
        vec{0, 1, 1},
        vec{1, 0, 0},
        vec{1, 0, 1},
        vec{1, 1, 0},
        vec{1, 1, 1}
    }) {

        vec result = generatePerpendicularVector(vector);

        CAPTURE(vector);
        // Vector is not 0
        REQUIRE_FALSE(almostEqual(norm(result), 0.0));
        // Vectors are perpendicular
        CAPTURE(INVARIANT_FAILED);
        REQUIRE(almostEqual(dot(result, vector), 0.0));
    }
}

TEST_CASE("Generate perpendicular vector: general vector", "[generatePerpendicularVector]") {
    vec vector = {7, -5, 6};
    vec expected = {6, -6, -12};

    vec result = generatePerpendicularVector(vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    CAPTURE(INVARIANT_FAILED);
    REQUIRE(almostEqual(dot(result, vector), 0.0));
}

TEST_CASE("Get cross product matrix: zero vector", "[getCrossProductMatrix]") {
    vec vector = zeros<vec>(3);
    mat expected = zeros<mat>(3, 3);

    mat result = getCrossProductMatrix(vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
}

TEST_CASE("Get cross product matrix: general", "[getCrossProductMatrix]") {
    vec vector = {5, 8, -3};
    mat test_matrix = {
            {1, 2, 3},
            {6, 5, 4},
            {7, -5, 12}
    };
    mat expected = {
            {-0., 3., 8.},
            {-3., 0., -5.},
            {-8., 5., 0.}
    };

    mat result = getCrossProductMatrix(vector);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    mat result_invariant = test_matrix * result;

    for (int i = 0; i < 3; i++) {
        vec row = result.row(i).t();
        CAPTURE(row);
        CAPTURE(INVARIANT_FAILED);
        REQUIRE(approx_equal(cross(test_matrix.row(i).t(), vector), result_invariant.row(i).t(), "absdiff", TOLERANCE));
    }
}

TEST_CASE("Cross division matrix: zero vector raises", "[getCrossDivisionMatrix]") {
    vec vector {0, 0, 0};

    REQUIRE_THROWS_AS(getCrossDivisionMatrix(vector), std::logic_error);
}

TEST_CASE("Cross division matrix: general", "[getCrossDivisionMatrix]") {
    vec divisor {5, 8, -3};
    mat test_matrix {
        {1, 2, 7},
        {6, 5, 23.33333333333333333},
        // not perpendicular to divisor
        {7, -5, 12}
    };
    mat expected {
        {0, 0.030612, 0.081633},
        {-0.030612, 0, -0.05102},
        {-0.081633, 0.05102, 0}
    };

    mat result = getCrossDivisionMatrix(divisor);

    REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));

    mat test_invariant = test_matrix * result;

    for (int i = 0; i < 3; ++i) {
        vec row = result.row(i).t();

        try {
            vec expectedResult = crossDivision(test_matrix.row(i).t(), divisor);
            CAPTURE(row);
            CAPTURE(INVARIANT_FAILED);
            REQUIRE(approx_equal(expectedResult, test_invariant.row(i).t(), "absdiff", TOLERANCE));
        }
        catch(const std::logic_error& e) {
            continue;
        }
    }
}

TEST_CASE("First input is zero raises exception", "[is_parallel]") {
    vec u = zeros<vec>(3);
    vec v = {4, 5, -2};

    REQUIRE_THROWS_AS(isParallel(u, v), std::logic_error);
}

TEST_CASE("Second input is zero raises exception", "[is_parallel]") {
    vec u = {4, 5, -2};
    vec v = zeros<vec>(3);

    REQUIRE_THROWS_AS(isParallel(u, v), std::logic_error);
}

TEST_CASE("Perpendicular vectors", "[is_parallel]") {
    vec u = {4, 5, -2};
    vec v = {5, 4, 20};

    bool result = isParallel(u, v);

    REQUIRE_FALSE(result);
}

TEST_CASE("Parallel vectors", "[is_parallel]") {
    vec u = {4, 5, -2};
    vec v = {8, 10, -4};

    bool result = isParallel(u, v);

    REQUIRE(result);
}

TEST_CASE("Anti parallel vectors", "[is_parallel]") {
    vec u = {4, 5, -2};
    vec v = {-8, -10, 4};

    bool result = isParallel(u, v);

    REQUIRE(result);
}

TEST_CASE("Almost parallel vectors", "[is_parallel]") {
    vec u = {4, 5, -2};
    vec v = {8, 10, -4.05};

    bool result = isParallel(u, v);

    REQUIRE_FALSE(result);
}