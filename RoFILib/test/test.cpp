#include "catch.hpp"
#include "../Configuration.h"
#include "../Printer.h"
#include "../visualizer/Visualizer.h"

TEST_CASE("Connections")
{
    for (Side side1 : {A, B})
    {
        for (Side side2 : {A, B})
        {
            for (Dock dock1 : {Zn, Xp, Xn})
            {
                for (Dock dock2 : {Zn, Xp, Xn})
                {
                    for (unsigned int ori : {0,1,2,3})
                    {
                        Configuration config;
                        config.addModule(0,0,0,0);
                        config.addModule(0,0,0,1);
                        config.addEdge({0, side1, dock1, ori, dock2, side2, 1});

                       // std::cout << side1 << " " << dock1 << " " << ori << " " << dock2 << " " << side2 << std::endl;

//                        config.computeRotations();
//                        Printer printer;
//                        std::cout << printer.printMatrices(config);
//
                        REQUIRE( config.isValid() );
                    }
                }
            }
        }
    }
}

TEST_CASE("Edge generation")
{
    Printer printer;
    //std::array<unsigned, 5> array = {0,0,0,0,0};
    Edge edge1(0, A, Xp, 0, Xp, A, 1);
    for (Side side2 : {A, B})
    {
        for (Dock dock2 : {Xp, Xn, Zn})
        {
            for (unsigned ori : {0,1,2,3})
            {
                for (Dock dock1 : {Xp, Xn, Zn})
                {
                    for (Side side1 : {A, B})
                    {
                        Edge edge2(0, side1, dock1, ori, dock2, side2, 1);
                        //std::cout << printer.print(edge) << printer.print(edge2) << std::endl;
                        REQUIRE(edge1 == edge2);

                        auto edgeOpt = nextEdge(edge1);
                        if (edgeOpt.has_value())
                            edge1 = edgeOpt.value();
                    }
                }
            }
        }
    }
}

TEST_CASE("Generate all edges")
{
    Printer p;

    Edge edge(0, A, Xp, 0, Xp, A, 1);
    auto edgeOpt = nextEdge(edge);
    while (edgeOpt.has_value())
    {
        edge = edgeOpt.value();
        edgeOpt = nextEdge(edge);
    }
}

TEST_CASE("Next configurations")
{
    Printer p;
    Configuration cfg;
    cfg.addModule(0,0,0,0);

    auto nextCfgs = cfg.next(90, 2);

    for (auto& next : nextCfgs)
    {
        //std::cout << p.print(next);
    }
}

TEST_CASE("Generate edge")
{
    Printer p;
    std::unordered_map<ID, std::array<bool, 6>> occupied;
    occupied[0] = {true, true, true, true, true, true};
    occupied[1] = {true, true, true, true, true, true};

    for (int i = 0; i < 6; ++i)
    {
        auto edge = generateEdge(0, 1, occupied);
        REQUIRE(edge.has_value());
    }
    auto edge = generateEdge(0, 1, occupied);
    REQUIRE(!edge.has_value());
}

TEST_CASE("Generate angles")
{
    std::vector<ID> ids = {0, 1};
    std::vector<Edge> edges;
    edges.emplace_back(0, A, Xp, 0, Zn, B, 1);

    auto cfg = generateAngles(ids, edges);
    REQUIRE(cfg.has_value());
    Printer p;
    //std::cout << p.print(cfg.value());
}

TEST_CASE("Generate angles for generated edges")
{
    Printer p;
    std::vector<ID> ids = {0, 1};
    std::unordered_map<ID, std::array<bool, 6>> occupied;
    occupied[0] = {true, true, true, true, true, true};
    occupied[1] = {true, true, true, true, true, true};

    for (int i = 0; i < 6; ++i)
    {
        auto edge = generateEdge(0, 1, occupied);
        REQUIRE(edge.has_value());
        std::vector<Edge> edges;
        edges.push_back(edge.value());
        auto cfg = generateAngles(ids, edges);
        REQUIRE(cfg.has_value());
        //std::cout << p.print(cfg.value());
    }
}

TEST_CASE("Sample free")
{
    Printer p;
    std::vector<ID> ids = {0, 1};
    std::optional<Configuration> cfg = sampleFree(ids);
    REQUIRE(cfg.has_value());
}

TEST_CASE("Sample free complicated")
{
    std::vector<ID> ids = {0, 1, 5, 7, 4};
    std::optional<Configuration> cfg = sampleFree(ids);
    while (!cfg.has_value())
    {
        cfg = sampleFree(ids);
    }
    REQUIRE(cfg.has_value());
//    Printer p;
//    std::cout << p.print(cfg.value());
//    Visualizer vis;
//    vis.drawConfiguration(cfg.value());
}