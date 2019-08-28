#include <catch.hpp>
#include <Camera.h>
#include <Generator.h>
#include <Visualizer.h>

TEST_CASE("Interpolate camera"){
    SECTION("Interpolate position"){
        Camera cameraStart;
        Camera cameraEnd;
        cameraStart.setPos(5, 5, 10);
        cameraEnd.setPos(-5, -5, 10);
        cameraStart.setFoc(0, 0, 0);
        cameraStart.setFoc(0, 0, 0);
        unsigned int steps = 10;
        for (unsigned int i = 0; i <= steps; i++) {
            Camera currentCamera = interpolateCamera(cameraStart, cameraEnd, i, steps);
            double distance = vecSize(currentCamera.getPos(), currentCamera.getFoc());
            double startDistance = vecSize(cameraStart.getPos(), cameraStart.getFoc());
            double endDistance = vecSize(cameraStart.getPos(), cameraEnd.getFoc());
            REQUIRE(distance == countSteps(startDistance, endDistance, i, steps));
            REQUIRE(currentCamera.getFoc()[0] == 0);
            REQUIRE(currentCamera.getFoc()[1] == 0);
            REQUIRE(currentCamera.getFoc()[2] == 0);
        }
        Camera cam = interpolateCamera(cameraStart, cameraEnd, 5, 10);
        REQUIRE(cam.getPos()[0] == 0);
        REQUIRE(cam.getPos()[1] == 0);
    }
    SECTION("Interpolate focus"){
        Camera cameraStart;
        Camera cameraEnd;
        cameraStart.setPos(0, 0, 0);
        cameraEnd.setPos(0, 0, 0);
        cameraStart.setFoc(5, 5, 10);
        cameraEnd.setFoc(-5, -5, 10);
        unsigned int steps = 10;
        for (unsigned int i = 0; i <= steps; i++) {
            Camera currentCamera = interpolateCamera(cameraStart, cameraEnd, i, steps);
            double distance = vecSize(currentCamera.getPos(), currentCamera.getFoc());
            double startDistance = vecSize(cameraStart.getPos(), cameraStart.getFoc());
            double endDistance = vecSize(cameraEnd.getPos(), cameraEnd.getFoc());
            REQUIRE(distance == countSteps(startDistance, endDistance, i, steps));
        }
        Camera cam = interpolateCamera(cameraStart, cameraEnd, 5, 10);
        REQUIRE(cam.getPos()[0] == 0);
        REQUIRE(cam.getPos()[1] == 0);
        REQUIRE(cam.getFoc()[0] == 0);
        REQUIRE(cam.getFoc()[1] == 0);
        REQUIRE(cam.getFoc()[2] == 10);
    }
    SECTION("Move position and focus parallel"){
        Camera cameraStart;
        Camera cameraEnd;
        cameraStart.setPos(5, 5, 5);
        cameraEnd.setPos(-5, -5, -5);
        cameraStart.setFoc(5, 5, 0);
        cameraEnd.setFoc(-5, -5, -10);
        unsigned int steps = 10;
        for (unsigned int i = 0; i <= steps; i++) {
            Camera currentCamera = interpolateCamera(cameraStart, cameraEnd, i, steps);
            double distance = vecSize(currentCamera.getPos(), currentCamera.getFoc());
            REQUIRE(distance == 5);
        }
        Camera cam = interpolateCamera(cameraStart, cameraEnd, 5, 10);
        REQUIRE(cam.getPos()[0] == 0);
        REQUIRE(cam.getPos()[1] == 0);
        REQUIRE(cam.getPos()[2] == 0);

        REQUIRE(cam.getFoc()[0] == 0);
        REQUIRE(cam.getFoc()[1] == 0);
        REQUIRE(cam.getFoc()[2] == -5);
    }
}

TEST_CASE("Visualizer simple generate"){
    Configuration cfg1, cfg2;
    cfg1.addModule(0,0,0, 0);
    cfg1.addModule(0,0,0, 1);
    cfg1.addModule(0,0,0, 2);
    Edge e1(0, static_cast<Side>(0), static_cast<Dock>(0), 2, static_cast<Dock>(1), static_cast<Side>(0), 1);
    Edge e2(1, static_cast<Side>(0), static_cast<Dock>(0), 2, static_cast<Dock>(1), static_cast<Side>(0), 2);
    cfg1.addEdge(e1);
    cfg1.addEdge(e2);


    cfg2.addModule(0,90,0, 0);
    cfg2.addModule(0,10,0, 1);
    cfg2.addModule(0,0,0, 2);
    Edge e3(0, static_cast<Side>(0), static_cast<Dock>(0), 2, static_cast<Dock>(1), static_cast<Side>(0), 1);
    Edge e4(1, static_cast<Side>(0), static_cast<Dock>(0), 2, static_cast<Dock>(1), static_cast<Side>(0), 2);
    cfg2.addEdge(e3);
    cfg2.addEdge(e4);

    SECTION("Only angle change") {
        std::vector<Configuration> generatedConfigs;
        double maxPhi = 30;
        unsigned int reconnection = 8;
        Generator generator;
        generator.generate(cfg1, cfg2, generatedConfigs, maxPhi, reconnection);
        REQUIRE(generatedConfigs.size() == 2);

        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Beta) == 30);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Beta) == 60);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Gamma) == 0);

        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Gamma) == 0);
    }

    SECTION("Add edge") {
        Edge e5(1, static_cast<Side>(1), static_cast<Dock>(1), 2, static_cast<Dock>(0), static_cast<Side>(1), 2);
        cfg2.addEdge(e5);
        std::vector<Configuration> generatedConfigs;
        double maxPhi = 30;
        unsigned int reconnection = 8;
        Generator generator;
        generator.generate(cfg1, cfg2, generatedConfigs, maxPhi, reconnection);
        REQUIRE(generatedConfigs.size() == 7);

        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Beta) == 30);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Beta) == 60);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Gamma) == 0);

        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Gamma) == 0);

        for (int i : {2, 3, 4, 5, 6}){
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Alpha) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Beta) == 90);
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Gamma) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Alpha) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Beta) == 10);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Gamma) == 0);
        }

        for (int i = 0; i < 7; i++) {
            std::vector<Edge> edges = generatedConfigs[i].getEdges(1);
            for (const auto &e : edges) {
                if (e == e5) {
                    REQUIRE(e.onCoeff() == 1/static_cast<double>(8) * (i + 1));
                } else {
                    REQUIRE(e.onCoeff() == 1);
                }
            }
        }
    }

    SECTION("Remove edge"){
        Edge e5(1, static_cast<Side>(1), static_cast<Dock>(1), 2, static_cast<Dock>(0), static_cast<Side>(1), 2);
        cfg1.addEdge(e5);
        std::vector<Configuration> generatedConfigs;
        double maxPhi = 30;
        unsigned int reconnection = 8;
        Generator generator;
        generator.generate(cfg1, cfg2, generatedConfigs, maxPhi, reconnection);
        REQUIRE(generatedConfigs.size() == 7);

        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Beta) == 30);
        REQUIRE(generatedConfigs[0].getModules().at(0).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Beta) == 60);
        REQUIRE(generatedConfigs[1].getModules().at(0).getJoint(Gamma) == 0);

        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[0].getModules().at(1).getJoint(Gamma) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Alpha) == 0);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Beta) == 10);
        REQUIRE(generatedConfigs[1].getModules().at(1).getJoint(Gamma) == 0);

        for (int i : {2, 3, 4, 5, 6}){
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Alpha) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Beta) == 90);
            REQUIRE(generatedConfigs[i].getModules().at(0).getJoint(Gamma) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Alpha) == 0);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Beta) == 10);
            REQUIRE(generatedConfigs[i].getModules().at(1).getJoint(Gamma) == 0);
        }

        for (int i = 0; i < 7; i++) {
            std::vector<Edge> edges = generatedConfigs[i].getEdges(1);
            for (const auto &e : edges) {
                if (e == e5) {
                    REQUIRE(e.onCoeff() == 1 - 1/static_cast<double>(8) * (i + 1));
                } else {
                    REQUIRE(e.onCoeff() == 1);
                }
            }
        }
    }
}