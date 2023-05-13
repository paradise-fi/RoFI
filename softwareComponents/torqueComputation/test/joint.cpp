#include <catch2/catch.hpp>
#include <armadillo>

#include "../src/joint.hpp"
#include "../src/variableManager.hpp"

using namespace arma;
using namespace rofi::torqueComputation;

const double TOLERANCE = 1e-6;


TEST_CASE("Joint initialization", "[Joint]") {
    Joint wall(
        0,
        {0, 1, 0},
        true,
        std::optional<vec>({1, 3, -2})
    );

    Joint motor_gamma(
        1,
        {1, 1, 0},
        {1, 0, 0}
    );

    Joint motor_alpha(
        2,
        {2, 1, 0},
        {0, 0, 1}
    );

    Joint rigid(
        3,
        {3, 1, 0},
        false,
        std::optional<vec>({5, -2, 4})
    );

    Joint end(
        4,
        {3, 1, 1},
        false,
        std::optional<vec>({2, -5, 9})
    );

    Joint lonely(
        42,
        {-1, -1, -1},
        false,
        std::optional<vec>({0, 1, 0})
    );

    //set joint neighbors
    wall.appendNeighbor(&motor_gamma);
    
    motor_gamma.appendNeighbor(&motor_alpha);
    motor_gamma.appendNeighbor(&wall);

    motor_alpha.appendNeighbor(&motor_gamma);
    motor_alpha.appendNeighbor(&rigid);

    rigid.appendNeighbor(&motor_alpha);
    rigid.appendNeighbor(&end);
    
    end.appendNeighbor(&rigid);

    //set joint forces
    rigid.appendForce({1, -2, 3});
    motor_gamma.appendForce({-1, 5, 7});
    end.appendForce({0, -5, 0});

    // init variables
    VariableManager variableManager;
    variableManager.addEdge(wall.getId(), motor_gamma.getId());
    variableManager.addEdge(motor_gamma.getId(), motor_alpha.getId());
    variableManager.addEdge(rigid.getId(), motor_alpha.getId());
    variableManager.addEdge(rigid.getId(), end.getId());

    SECTION("Wall initialization") {
        REQUIRE(wall.getId() == 0);
        REQUIRE_FALSE(wall.getIsBounded());

        vec expected_norm = 0.267261 * vec{1, 3, -2};
        REQUIRE(approx_equal(expected_norm, wall.getNorm().value(), "absdiff", TOLERANCE));

        vec expected_coors = {0, 1, 0};
        REQUIRE(approx_equal(expected_coors, wall.getCoors(), "absdiff", TOLERANCE));

        REQUIRE(wall.getIsWall());

        mat expected_moments = {
                {0.408248, 0.408248, 0.816497},
                {0.872872, -0.436436, -0.218218},
                {0.267261, 0.801784, -0.534522}
        };

        REQUIRE(approx_equal(expected_moments, wall.getMoments(), "absdiff", TOLERANCE));
        REQUIRE(wall.getForces().size() == 0);
        REQUIRE(wall.getNeighbors() == std::vector<Joint*>{&motor_gamma});
    }

    SECTION("Motor gamma initialization") {
        REQUIRE(motor_gamma.getId() == 1);
        REQUIRE(motor_gamma.getIsBounded());

        vec expected_norm = {1, 0, 0};
        REQUIRE(approx_equal(motor_gamma.getNorm().value(), expected_norm, "absdiff", TOLERANCE));

        vec expected_coors = {1, 1, 0};
        REQUIRE(approx_equal(motor_gamma.getCoors(), expected_coors, "absdiff", TOLERANCE));

        REQUIRE_FALSE(motor_gamma.getIsWall());

        mat expected_moments = {
            {0., 0., -1.},
            {-0., 1., 0.},
            {1., 0., 0.}
        };
        REQUIRE(approx_equal(motor_gamma.getMoments(), expected_moments, "absdiff", TOLERANCE));

        REQUIRE(motor_gamma.getForces().size() == 1);
        REQUIRE(approx_equal(motor_gamma.getForces()[0], vec{-1, 5, 7}, "absdiff", TOLERANCE));

        REQUIRE(motor_gamma.getNeighbors() == std::vector<Joint*>({&motor_alpha, &wall}));
    }

    SECTION("Motor alpha initialization") {
        REQUIRE(motor_alpha.getId() == 2);
        REQUIRE(motor_alpha.getIsBounded());

        vec expected_norm = {0, 0, 1};
        REQUIRE(approx_equal(motor_alpha.getNorm().value(), expected_norm, "absdiff", TOLERANCE));

        vec expected_coors = {2, 1, 0};
        REQUIRE(approx_equal(motor_alpha.getCoors(), expected_coors, "absdiff", TOLERANCE));

        REQUIRE_FALSE(motor_alpha.getIsWall());

        mat expected_moments = {
            {0.707107, 0.707107, 0},
            {-0.707107, 0.707107, 0},
            {0, 0, 1}
        };
        REQUIRE(approx_equal(motor_alpha.getMoments(), expected_moments, "absdiff", TOLERANCE));

        REQUIRE(motor_alpha.getForces().size() == 0);

        REQUIRE(motor_alpha.getNeighbors() == std::vector<Joint*>({&motor_gamma, &rigid}));
    }

    SECTION("Rigid joint initialization") {
        REQUIRE(rigid.getId() == 3);
        REQUIRE_FALSE(rigid.getIsBounded());

        vec expected_norm = 0.1490712 * vec{5, -2, 4};
        REQUIRE(approx_equal(rigid.getNorm().value(), expected_norm, "absdiff", TOLERANCE));

        vec expected_coors = {3, 1, 0};
        REQUIRE(approx_equal(rigid.getCoors(), expected_coors, "absdiff", TOLERANCE));

        REQUIRE_FALSE(rigid.getIsWall());

        mat expected_moments = {
            {0.444444, -0.444444, -0.777778},
            {0.496904, 0.844737, -0.198762},
            {0.745356, -0.298142, 0.596285}
        };
        REQUIRE(approx_equal(rigid.getMoments(), expected_moments, "absdiff", TOLERANCE));

        REQUIRE(rigid.getForces().size() == 1);
        REQUIRE(approx_equal(rigid.getForces()[0], vec{1, -2, 3}, "absdiff", TOLERANCE));
        
        REQUIRE(rigid.getNeighbors() == std::vector<Joint*>({&motor_alpha, &end}));
    }

    SECTION("End initialization") {
        REQUIRE(end.getId() == 4);
        REQUIRE_FALSE(end.getIsBounded());

        vec expected_norm = 0.095346259 * vec{2, -5, 9};
        REQUIRE(approx_equal(end.getNorm().value(), expected_norm, "absdiff", TOLERANCE));
        
        vec expected_coors = {3, 1, 1};
        REQUIRE(approx_equal(end.getCoors(), expected_coors, "absdiff", TOLERANCE));
        
        REQUIRE_FALSE(end.getIsWall());

        mat expected_moments = {
            {0.619586, -0.619586, -0.4819},
            {0.761413, 0.623571, 0.177225},
            {0.190693, -0.476731, 0.858116}
        };
        REQUIRE(approx_equal(end.getMoments(), expected_moments, "absdiff", TOLERANCE));

        REQUIRE(end.getForces().size() == 1);
        REQUIRE(approx_equal(end.getForces()[0], vec{0, -5, 0}, "absdiff", TOLERANCE));

        REQUIRE(end.getNeighbors() == std::vector<Joint*>({&rigid}));
    }

    SECTION("Sanity check") {
        for (auto joint : {&wall, &motor_gamma, &motor_alpha, &rigid, &end, &lonely}) {
            REQUIRE_NOTHROW(joint->jointSanityCheck());
        }
    }

    SECTION("get_ momentForces_not_neighbors_raises") {
        REQUIRE_THROWS_AS(lonely.getMomentForces(wall), std::logic_error);
    }

    SECTION("get_ momentForces_wall_gamma") {
        mat expected = {
            {0., 0.816497, -0.408248},
            {0., -0.218218, 0.436436},
            {0., -0.534522, -0.801784}
        };
        vec expected_x_coors = zeros<vec>(3);

        mat result = wall.getMomentForces(motor_gamma);

        REQUIRE(approx_equal(result, expected, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(result.col(0), expected_x_coors, "absdiff", TOLERANCE));
    }

    SECTION("test_get_ momentForces_gamma_wall") {
        mat expected = {
            {0., 1., 0.},
            {0., 0., 1.},
            {0., 0., 0.}
        };
        vec expected_x_coors = zeros<vec>(3);

        mat result = motor_gamma.getMomentForces(wall);

        REQUIRE(approx_equal(result, expected, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(result.col(0), expected_x_coors, "absdiff", TOLERANCE));
    }

    SECTION("test_get_ momentForces_rigid_end") {
        mat expected = {
            {-0.444444, -0.444444, 0.},
            {0.844737, -0.496904, 0.},
            {-0.298142, -0.745356, 0.}
        };
        vec expected_z_coors = zeros<vec>(3);

        mat result = rigid.getMomentForces(end);

        REQUIRE(approx_equal(result, expected, "absdiff", 1e-6));
        REQUIRE(approx_equal(result.col(2), expected_z_coors, "absdiff", TOLERANCE));
    }

    SECTION("test_getNeighbourById_lonely") {
        Joint* result = lonely.getNeighbourById(0);

        REQUIRE(result == nullptr);
    }

    SECTION("test_getNeighbourById_own_id") {
        Joint* result = wall.getNeighbourById(0);

        REQUIRE(result == nullptr);
    }

    SECTION("test_getNeighbourById_invalid_id") {
        Joint* result = wall.getNeighbourById(69);

        REQUIRE(result == nullptr);
    }

    SECTION("test_getNeighbourById_not_neighbors") {
        Joint* result = wall.getNeighbourById(3);

        REQUIRE(result == nullptr);
    }

    SECTION("test_getNeighbourById_not_neighbors") {
        Joint* result = wall.getNeighbourById(1);

        REQUIRE(*result == motor_gamma);
    }

    SECTION("test_getNeighbourById_more_neighbors_found") {
        Joint* result = motor_gamma.getNeighbourById(2);

        REQUIRE(*result == motor_alpha);
    }

    SECTION("test_getCanonicNeighborsIds_lonely") {
        std::vector<int> result = lonely.getCanonicNeighborsIds();

        REQUIRE(result.size() == 0);
    }

    SECTION("test_getCanonicNeighborsIds_correct_order") {
        std::vector<int> expected = {0, 2};

        std::vector<int> result = motor_gamma.getCanonicNeighborsIds();

        REQUIRE(result == expected);
    }

    SECTION("test_createForceLhs_wall") {
        auto expected = std::nullopt;

        auto result = wall.createForceLhs(variableManager);

        REQUIRE(result == expected);
    }

    SECTION("test_createForceLhs_motor_gamma") {
        mat expected = {
            {0., 0., 0., -1., 0., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0.81649658, -0.21821789, -0.53452248, 0., -1., 0., 0., 1., 0., 0., 0., 0., 0., -1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {-0.40824829, 0.43643578, -0.80178373, 0., 0., -1., 0., 0., 1., 0., 0., 0.70710678, 0.70710678, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.}
        };

        mat result = motor_gamma.createForceLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createForceLhs_motor_alpha") {
        mat expected = {
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., -1., 0., 0., 0., 0., 0., 0., 1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., 0., 0., 0., -1., 0., 0., 0., 0., 0., 1., 0.77777778, 0.1987616, -0.59628479, 0., 0., 0., -1., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., 0., 0., 0., 0., -1., 0., 0., -0.70710678, -0.70710678, 0., -0.44444444, 0.84473679, -0.2981424, 0., 0.70710678, 0.70710678, 0., 0., 0., 0., 0., 0., 0., 0.}
        };

        mat result = motor_alpha.createForceLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createForceLhs_rigid") {
        mat expected = {
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., -1., 0., 0., 0., 0.44444444, -0.84473679, 0.2981424, 0., 0.61958555, -0.62357118, 0.47673129},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., -0.77777778, -0.1987616, 0.59628479, 0., 0., 0., 1., 0.44444444, 0.49690399, 0.74535599, 0., 0.61958555, 0.76141323, 0.19069252},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.44444444, -0.84473679, 0.2981424, 0., -0.70710678, -0.70710678, 0., 0., 0., 0., 1., 0., 0., 0.}
        };

        mat result = rigid.createForceLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createForceLhs_end") {
        mat expected = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.44444444, 0.84473679, -0.2981424, 0.0, -0.61958555, 0.62357118, -0.47673129},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.44444444, -0.49690399, -0.74535599, 0.0, -0.61958555, -0.76141323, -0.19069252},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -1, 0.0, 0.0, 0.0}
        };

        mat result = end.createForceLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentLhs_wall") {
        auto expected = std::nullopt;

        auto result = wall.createMomentLhs(variableManager);

        REQUIRE(result == expected);
    }

    SECTION("test_createMomentLhs_motor_gamma") {
        mat expected = {
            {0., 0., 0., 0., 0., 0., 1., 0., 0., 1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., 0., 1., 0., 0., 1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., -1., 0., 0., -1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.}
        };

        mat result = motor_gamma.createMomentLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentLhs_motor_alpha") {
        mat expected = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.70710678, -0.70710678, 0.0, 0.0, 0.0, 0.0, 0.0, 0.70710678, -0.70710678, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.70710678, 0.70710678, 0.0, 0.0, 0.0, 0.0, 0.0, 0.70710678, 0.70710678, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
        };

        mat result = motor_alpha.createMomentLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentLhs_rigid") {
        mat expected = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.44444444, 0.49690399, 0.74535599, 0.0, 0.0, 0.0, 0.0, 0.44444444, 0.49690399, 0.74535599, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.44444444, 0.84473679, -0.2981424, 0.0, 0.0, 0.0, 0.0, -0.44444444, 0.84473679, -0.2981424, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.77777778, -0.1987616, 0.59628479, 0.0, 0.0, 0.0, 0.0, -0.77777778, -0.1987616, 0.59628479, 0.0, 0.0, 0.0, 0.0}
        };

        mat result = rigid.createMomentLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentLhs_end") {
        mat expected = {
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.61958555, 0.76141323, 0.19069252},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., -0.61958555, 0.62357118, -0.47673129},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., -0.48189987, 0.17722549, 0.85811633}
        };

        mat result = end.createMomentLhs(variableManager).value();

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentEdgeLhs_wall__motor_gamma") {
        mat expected = {
            {0.40824829, 0.87287156, 0.26726124, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
        };

        mat result = wall.createMomentEdgeLhs(motor_gamma, variableManager);
        mat result_opposite = motor_gamma.createMomentEdgeLhs(wall, variableManager);

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(-expected, result_opposite, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentEdgeLhs_motor_gamma__motor_alpha") {
        mat expected = {
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 1., 0., -0.70710678, 0.70710678, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.},
            {0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.}
        };

        mat result = motor_gamma.createMomentEdgeLhs(motor_alpha, variableManager);
        mat result_opposite = motor_alpha.createMomentEdgeLhs(motor_gamma, variableManager);

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(-expected, result_opposite, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentEdgeLhs_motor_alpha__rigid") {
        mat expected = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.44444444, -0.49690399, -0.74535599, 0.0, 0.70710678, -0.70710678, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}
        };

        mat result = motor_alpha.createMomentEdgeLhs(rigid, variableManager);
        mat result_opposite = rigid.createMomentEdgeLhs(motor_alpha, variableManager);

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(-expected, result_opposite, "absdiff", TOLERANCE));
    }

    SECTION("test_createMomentEdgeLhs_rigid__end") {
        mat expected = {
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, -0.77777778, -0.1987616, 0.59628479, 0.0, 0.48189987, -0.17722549, -0.85811633}
        };

        mat result = rigid.createMomentEdgeLhs(end, variableManager);
        mat result_opposite = end.createMomentEdgeLhs(rigid, variableManager);

        REQUIRE(approx_equal(expected, result, "absdiff", TOLERANCE));
        REQUIRE(approx_equal(-expected, result_opposite, "absdiff", TOLERANCE));
    }
}
