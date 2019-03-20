#include "catch.hpp"
#include "../Configuration.h"
#include "../Printer.h"

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
    std::array<unsigned, 5> array = {0,0,0,0,0};
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
                        Edge edge = arrayToEdge(0, 1, array);
                        Edge edge2(0, side1, dock1, ori, dock2, side2, 1);
                        //std::cout << printer.print(edge) << printer.print(edge2) << std::endl;
                        REQUIRE(edge == edge2);
                        array = getNextArray(array);
                    }
                }
            }
        }
    }
}

TEST_CASE("Generate all edges")
{
    Printer p;
    const static std::array<unsigned, 5> init = {0,0,0,0,0};
    std::array<unsigned, 5> array = {0,0,0,0,0};
    do
    {
        Edge edge = arrayToEdge(0, 1, array);
        array = getNextArray(array);

        //std::cout << p.print(edge) << std::endl;
    } while (array != init);
}

TEST_CASE("Next configurations")
{
    Printer p;
    Configuration cfg;
    cfg.addModule(0,0,0,0);

    auto nextCfgs = cfg.next(90, 2);

    for (auto& next : nextCfgs)
    {
        std::cout << p.print(next);
    }
}