#include <catch2/catch.hpp>
#include <armadillo>

#include "../src/variableManager.hpp"

using namespace arma;
const double TOLERANCE = 1e-12;

TEST_CASE("addEdge: when already added raises", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);

    REQUIRE_THROWS_AS(mgr.addEdge(0, 1), std::logic_error);
}

TEST_CASE("getMomentStartIndexForEdge: invalid from raises", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);

    REQUIRE_THROWS_AS(mgr.getMomentStartIndexForEdge(-1, 1), std::out_of_range);
}

TEST_CASE("getMomentStartIndexForEdge: invalid to raises", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    REQUIRE_THROWS_AS(mgr.getMomentStartIndexForEdge(0, -1), std::out_of_range);
}

TEST_CASE("getMomentStartIndexForEdge: correct", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    int expected = 0;

    int result = mgr.getMomentStartIndexForEdge(0, 1);

    REQUIRE(expected == result);
}

TEST_CASE("getMomentStartIndexForEdge: opposite direction", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    int expected = 4;

    int result = mgr.getMomentStartIndexForEdge(1, 0);

    REQUIRE(expected == result);
}

TEST_CASE("getForceReactionIndexForEdge: invalid from raises", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);

    REQUIRE_THROWS_AS(mgr.getForceReactionIndexForEdge(-1, 1), std::out_of_range);
}

TEST_CASE("getForceReactionIndexForEdge: invalid to raises", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);

    REQUIRE_THROWS_AS(mgr.getForceReactionIndexForEdge(0, -1), std::out_of_range);
}

TEST_CASE("getForceReactionIndexForEdge: correct", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    int expected = 3;

    int result = mgr.getForceReactionIndexForEdge(0, 1);

    REQUIRE(expected == result);
}

TEST_CASE("getForceReactionIndexForEdge: opposite direction", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    int expected = 3;
    
    int result = mgr.getForceReactionIndexForEdge(1, 0);
    
    REQUIRE(expected == result);
}

TEST_CASE("getVariablesCount: empty", "[VariableManager]") {
    VariableManager mgr;
    int expected = 0;

    int result = mgr.getVariablesCount();

    REQUIRE(expected == result);
}

TEST_CASE("getVariablesCount: correct count", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    mgr.addEdge(1, 2);
    mgr.addEdge(0, 2);
    int expected = 21;

    int result = mgr.getVariablesCount();

    REQUIRE(expected == result);
}

TEST_CASE("getVariablesLowerBounds: empty", "[VariableManager]") {
    VariableManager mgr;
    uword expected = 0;

    vec result = mgr.getVariablesLowerBounds();

    REQUIRE(expected == result.size());
}

TEST_CASE("getVariablesLowerBounds: without bounds", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    vec expected(7, fill::value(-INFINITY));
    uword expected_len = 7;

    vec result = mgr.getVariablesLowerBounds();

    REQUIRE(result.n_elem == expected_len);
    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesLowerBounds: edge with bounds only from", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, 1);
    vec expected = {
            -INFINITY, -INFINITY, -1, -INFINITY, -INFINITY, -INFINITY, -INFINITY
    };
    
    vec result = mgr.getVariablesLowerBounds();
    
    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesLowerBounds: edge with bounds only to", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, std::nullopt, 5);
    vec expected = {
            -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -INFINITY, -5
    };

    vec result = mgr.getVariablesLowerBounds();

    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesLowerBounds: edge with bounds both ends bounded", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, 1, 5);
    vec expected = {
            -INFINITY, -INFINITY, -1, -INFINITY, -INFINITY, -INFINITY, -5
    };

    vec result = mgr.getVariablesLowerBounds();

    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesUpperBounds: empty", "[VariableManager]") {
    VariableManager mgr;
    uword expected = 0;

    vec result = mgr.getVariablesUpperBounds();

    REQUIRE(expected == result.size());
}

TEST_CASE("getVariablesUpperBounds: without bounds", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1);
    vec expected(7, fill::value(INFINITY));
    uword expected_len = 7;

    vec result = mgr.getVariablesUpperBounds();

    REQUIRE(result.n_elem == expected_len);
    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesUpperBounds: edge with bounds only from", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, 1);
    vec expected = {
            INFINITY, INFINITY, 1, INFINITY, INFINITY, INFINITY, INFINITY
    };

    vec result = mgr.getVariablesUpperBounds();

    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesUpperBounds: edge with bounds only to", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, std::nullopt, 5);
    vec expected = {
            INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, INFINITY, 5
    };

    vec result = mgr.getVariablesUpperBounds();

    REQUIRE(all(expected == result));
}

TEST_CASE("getVariablesUpperBounds: edge with bounds both ends bounded", "[VariableManager]") {
    VariableManager mgr;
    mgr.addEdge(0, 1, 1, 5);
    vec expected = {
            INFINITY, INFINITY, 1, INFINITY, INFINITY, INFINITY, 5
    };

    vec result = mgr.getVariablesUpperBounds();

    REQUIRE(all(expected == result));
}