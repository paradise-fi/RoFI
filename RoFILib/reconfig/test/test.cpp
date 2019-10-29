#include <catch.hpp>
#include <Configuration.h>
#include <Algorithms.h>

TEST_CASE("Connections")
{
    for (ShoeId side1 : {A, B})
    {
        for (ShoeId side2 : {A, B})
        {
            for (ConnectorId dock1 : {ZMinus, XPlus, XMinus})
            {
                for (ConnectorId dock2 : {ZMinus, XPlus, XMinus})
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
    //std::array<unsigned, 5> array = {0,0,0,0,0};
    Edge edge1(0, A, XPlus, 0, XPlus, A, 1);
    for (ShoeId side2 : {A, B})
    {
        for (ConnectorId dock2 : {XPlus, XMinus, ZMinus})
        {
            for (unsigned ori : {0,1,2,3})
            {
                for (ConnectorId dock1 : {XPlus, XMinus, ZMinus})
                {
                    for (ShoeId side1 : {A, B})
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
    Edge edge(0, A, XPlus, 0, XPlus, A, 1);
    auto edgeOpt = nextEdge(edge);
    while (edgeOpt.has_value())
    {
        edge = edgeOpt.value();
        edgeOpt = nextEdge(edge);
    }
}

TEST_CASE("Get all subsets")
{
    std::vector<int> vals = {1,2,3,4};
    std::vector<std::vector<int>> res1;
    getAllSubsets(vals, res1, 0);
    REQUIRE(res1.size() == 1);

    std::vector<std::vector<int>> res2;
    getAllSubsets(vals, res2, 1);
    REQUIRE(res2.size() == 5);

    std::vector<std::vector<int>> res3;
    getAllSubsets(vals, res3, 2);
    REQUIRE(res3.size() == 11);
}

TEST_CASE("Generate all rotations without repetitions")
{
    Rots rots;
    rots.push_back({0,Alpha,90});
    rots.push_back({0,Alpha,-90});
    rots.push_back({0,Beta,90});
    rots.push_back({1,Beta,90});

    std::vector<Rots> res;
    getAllRots(rots, res, 3);


    REQUIRE(res.size() == 2 + 4 + 1 + 5);
}

TEST_CASE("Generate all actions")
{
    Configuration cfg;
    cfg.addModule(0,0,0,0);

    std::vector<Action> res2;
    cfg.generateActions(res2, 90, 1);

    REQUIRE( res2.size() == 7 );

    std::vector<Action> res;
    cfg.generateActions(res, 90, 2);
    REQUIRE( res.size() == 19 );
}

TEST_CASE("Next configurations")
{
    Configuration cfg;
    cfg.addModule(0,0,0,0);

    std::vector<Configuration> nextCfgs;
    cfg.next(nextCfgs, 90, 2);

    for (auto& next : nextCfgs)
    {
        //std::cout << p.print(next);
    }
}

TEST_CASE("Generate edge")
{
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
    edges.emplace_back(0, A, XPlus, 0, ZMinus, B, 1);

    auto cfg = generateAngles(ids, edges);
    REQUIRE(cfg.has_value());
    //std::cout << p.print(cfg.value());
}

TEST_CASE("Generate angles for generated edges")
{
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


TEST_CASE("Diff only rotations - one change") {
    Configuration cfg1, cfg2;

    cfg1.addModule(0, 0, 0, 0);
    cfg2.addModule(90, 0, 0, 0);

    Action action = cfg1.diff(cfg2);
    REQUIRE(action.reconnections().empty());
    REQUIRE(action.rotations().size() == 1);
    REQUIRE(action.rotations().at(0) == Action::Rotate(0, Joint::Alpha, 90));
}

TEST_CASE("Diff only rotations - more changes") {
    Configuration cfg1, cfg2;

    cfg1.addModule(0, 0, 0, 0);
    cfg2.addModule(90, -45, 0, 0);

    Action action = cfg1.diff(cfg2);
    REQUIRE(action.reconnections().empty());
    REQUIRE(action.rotations().size() == 2);
    REQUIRE(action.rotations().at(0) == Action::Rotate(0, Joint::Alpha, 90));
    REQUIRE(action.rotations().at(1) == Action::Rotate(0, Joint::Beta, -45));
}

TEST_CASE("Diff only reconnections") {
    Configuration cfg1, cfg2;
    Edge edge1(1, static_cast<ShoeId>(0), static_cast<ConnectorId>(0), 0, static_cast<ConnectorId>(0), static_cast<ShoeId>(1), 2);
    Edge edge2(0, static_cast<ShoeId>(1), static_cast<ConnectorId>(2), 1, static_cast<ConnectorId>(2), static_cast<ShoeId>(0), 1);

    cfg1.addModule(90, 90, 0, 0);
    cfg1.addModule(0, 90, 90, 1);
    cfg1.addModule(90, 0, 90, 2);
    cfg1.addEdge(edge1);
    cfg1.addEdge(edge2);

    Action::Reconnect reconnect1(false, edge1);
    cfg2 = cfg1;
    cfg2.execute({{}, {reconnect1}});
    Action action1 = cfg1.diff(cfg2);
    REQUIRE(action1.reconnections().size() == 1);
    REQUIRE(action1.rotations().empty());
    REQUIRE(action1.reconnections().at(0) == reconnect1);

    Action::Reconnect reconnect2(false, edge2);
    cfg2.execute({{}, {reconnect2}});
    Action action2 = cfg1.diff(cfg2);
    REQUIRE(action2.reconnections().size() == 2);
    REQUIRE(action2.rotations().empty());
    REQUIRE((action2.reconnections().at(0) == reconnect1 || action2.reconnections().at(0) == reconnect2));
    REQUIRE((action2.reconnections().at(1) == reconnect1 || action2.reconnections().at(1) == reconnect2));
    REQUIRE(action2.reconnections().at(0) != action2.reconnections().at(1));

    Action::Reconnect reconnect3(true, edge1);
    cfg2.execute({{}, {reconnect3}});
    Action action3 = cfg1.diff(cfg2);
    REQUIRE(action3.reconnections().size() == 1);
    REQUIRE(action3.rotations().empty());
    REQUIRE(action3.reconnections().at(0) == reconnect2);
}

TEST_CASE("Diff reconnections and rotations") {
    Configuration cfg1, cfg2;
    Edge edge1(1, static_cast<ShoeId>(0), static_cast<ConnectorId>(0), 0, static_cast<ConnectorId>(0), static_cast<ShoeId>(1), 2);
    Edge edge2(0, static_cast<ShoeId>(1), static_cast<ConnectorId>(2), 1, static_cast<ConnectorId>(2), static_cast<ShoeId>(0), 1);

    cfg1.addModule(90, 90, 0, 0);
    cfg1.addModule(0, 90, 90, 1);
    cfg1.addModule(90, 0, 90, 2);
    cfg1.addEdge(edge1);
    cfg1.addEdge(edge2);

    Action::Reconnect reconnect1(false, edge1);
    Action::Reconnect reconnect2(false, edge2);
    Action::Rotate rotate(1, Joint::Beta, -90);
    cfg2 = cfg1;
    cfg2.execute({{rotate}, {reconnect1, reconnect2}});
    Action action = cfg1.diff(cfg2);
    REQUIRE(action.rotations().size() == 1);
    REQUIRE(action.reconnections().size() == 2);
    REQUIRE(action.rotations().at(0) == rotate);
    REQUIRE((action.reconnections().at(0) == reconnect1 || action.reconnections().at(0) == reconnect2));
    REQUIRE((action.reconnections().at(1) == reconnect1 || action.reconnections().at(1) == reconnect2));
    REQUIRE(action.reconnections().at(0) != action.reconnections().at(1));
}



