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

TEST_CASE("Priority rotation")
{

}