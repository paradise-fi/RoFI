#include <exception>
#include <fstream>
#include <tuple>
#include <stdlib.h>
#include "../configuration/Configuration.h"
#include "../configuration/IO.h"
#include "Snake_algorithms.h"

bool subset(const std::unordered_map<ID, ShoeId>& map1, const std::unordered_map<ID, ShoeId>& map2) {
    bool isEq = true;
    for (const auto& [id, shoe] : map1) {
        if (map2.find(id) == map2.end()) {
            std::cout << id << " not in map2" << std::endl;
            isEq = false;
            continue;
        }
        if (map1.at(id) == map2.at(id))
            continue;
        std::cout << id << " : " << map1.at(id) << " != " << map2.at(id) << std::endl;
        isEq = false;
    }
    return isEq;
}


bool mapEq(const std::unordered_map<ID, ShoeId>& map1, const std::unordered_map<ID, ShoeId>& map2) {
    std::cout << "1 is subset 2?\n";
    auto res = subset(map1, map2);
    std::cout << "2 is subset 1?\n";
    auto res2 = subset(map2, map1);
    return res && res2;
}

void printVecToFile(const std::vector<Configuration>& configs, const char* path) {
    std::ofstream file;
    file.open(std::string(path));
    for (const auto& conf : configs) {
        file << IO::toString(conf) << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        throw std::invalid_argument("No filepath given");
    }
/*
    std::ifstream initInput;
    initInput.open(argv[1]);
    Configuration init;
    IO::readConfiguration(initInput, init);
    init.computeMatrices();
*/
/*
    {
        std::cout << "Loaded: \n" << IO::toString(init);
        auto sg = SpaceGrid(init);
        std::cout << "Freeness: " << std::endl << sg.getFreeness() << std::endl;
        sg.printGrid();
    }
    */
    /*
    std::vector<Configuration> path = fixLeaf(init, 3);
    std::cout << IO::toString(path[path.size()-1]) << std::endl;
    */
    /*
    AlgorithmStat stat;
    int moduleCount = init.getIDs().size();

    double path_pref = 0.5;
    int limit = 10000/moduleCount;

    if (argc > 3) {
        path_pref = std::atof(argv[2]);
        limit = std::atoi(argv[3]);
    }
*/
/* Testing generator speed */
/*
    auto allowed = makeAllowed(init, 88, 90);
    std::vector<Configuration> next;
    // smartBisimpleOnlyRotNext(init, next, 90, allowed);
    biParalyzedOnlyRotNext(init, next, 90, allowed);
    std::cout << next.size() << std::endl;
*/
/* Testing aerateConfig */
/*
    auto path = aerateConfig(init);
    std::cout << IO::toString(path.back()) << std::endl;;
*/
/*
    Testing treefy
    SpaceGrid debug(path[path.size()-1]);
    std::cout << debug.getFreeness() << std::endl;

    std::cout << stat.toString();

    std::cout << IO::toString(path[path.size()-1]);
    path.push_back(treefy<MakeStar>(path[path.size()-1]));
    std::cout << IO::toString(path[path.size()-1]);
*/
/*  Testing makeSpace */
/*
    std::unordered_set<ID> isolate;
    if (!init.computeSpanningTree())
        std::cout << "Compute failed" << std::endl;

    addSubtree(91, isolate, init.getSpanningSucc());
    std::cout << "Isolate size: " << isolate.size() << std::endl;
    path = makeSpace(init, isolate, &stat);
    std::cout << IO::toString(path[path.size()-1]) << std::endl;
*/

/* Testing subTreeSize */
/*
    std::unordered_map<ID, unsigned> subtreeSizesT;
    computeSubtreeSizes(init, subtreeSizesT);
    for (const auto& [id, size] : subtreeSizesT) {
        std::cout << id << " " << size << std::endl;
    }
    std::cout << "------------------------" << std::endl;
*/

/*  Testing findLeafs */
/*
    std::unordered_map<ID, ShoeId> leafsBlack;
    std::unordered_map<ID, ShoeId> leafsWhite;
    findLeafs(init, leafsBlack, leafsWhite);
    std::cout << "Black leafs" << std::endl;
    for (const auto& [id, shoe] : leafsBlack) {
        std::cout << "\t" << id << " " << shoe << std::endl;
    }
    std::cout << "White leafs" << std::endl;
    for (const auto& [id, shoe] : leafsWhite) {
        std::cout << "\t" << id << " " << shoe << std::endl;
    }
*/

/* Testing computeActiveRadiuses */
/*
    std::vector<std::pair<ID, ShoeId>> leafsBlack;
    std::vector<std::pair<ID, ShoeId>> leafsWhite;
    std::unordered_map<ID, std::tuple<ID, ShoeId, unsigned>> activeRadiuses;
    std::unordered_map<ID, unsigned> subtreeSizes;
    findLeafs(init, leafsBlack, leafsWhite);
    for (const auto& [id, shoe] : leafsWhite) {
        leafsBlack.emplace_back(id, shoe);
    }
    computeSubtreeSizes(init, subtreeSizes);
    computeActiveRadiuses(init, subtreeSizes, leafsBlack, activeRadiuses);
    for (const auto& [id, rad] : activeRadiuses) {
        std::cout << id << " " << std::get<2>(rad) << std::endl;
    }
    std::cout << "--------------\n";
*/

/* Testing makeEdgeSpace */
/*  Jakože vypadá celkem ok? Podívat se pak na ty čísla,
    co jsem se vůbec snažil vyprostoroval */
/*
    auto path = makeEdgeSpace(init, 123, 50);
    if (path.empty())
        std::cout << "Dafuq is empty" << std::endl;
    else
        std::cout << IO::toString(path.back()) <<std::endl;
*/

/* Testing connectArm */
/*
    std::unordered_map<ID, unsigned> subtreeSizes;
    std::unordered_map<ID, ShoeId> leafsBlack;
    std::unordered_map<ID, ShoeId> leafsWhite;
    const auto& matrices = init.getMatrices();
    computeSubtreeSizes(init, subtreeSizes);
    findLeafs(init, leafsBlack, leafsWhite);
    ID bid = 88;
    ID wid = 90;

    auto [r, _s, rad] = computeActiveRadius(init, subtreeSizes, bid, leafsBlack[bid]);
    auto [wr, w_s, wrad] = computeActiveRadius(init, subtreeSizes, wid, leafsWhite[wid]);
    unsigned rootDist = newyorkCenterDistance(matrices.at(r)[_s], matrices.at(wr)[w_s]);

    std::cout << rad << " " << wrad << " " << rootDist << std::endl;
    Edge dc(bid, leafsBlack[bid], ConnectorId::ZMinus, Orientation::North, ConnectorId::ZMinus, leafsWhite[wid], wid);
    auto res = connectArm(init, dc, r, wr);
    if (res.empty())
        std::cout << "Jsme hloupí" << std::endl;
    else {
        std::cout << IO::toString(res.back()) << std::endl;
        std::cout << IO::toString(disjoinArm(res.back(), dc, leafsBlack, leafsWhite)) << std::endl;

        std::unordered_map<ID, ShoeId> leafsBlack2;
        std::unordered_map<ID, ShoeId> leafsWhite2;
        findLeafs(res.back(), leafsBlack2, leafsWhite2);
        std::cout << "Blacks:\n";
        mapEq(leafsBlack, leafsBlack2);
        std::cout << "Whites:\n";
        mapEq(leafsWhite, leafsWhite2);
    }
*/
    /* NEED TO CHECK DISCONNECT ARM !!! */

/* Testing treeToSnake */
/*
    auto config = treefy<MakeStar>(init);
    auto res = treeToSnake(config);
    if (res.empty())
        std::cout << "Unsucc" << std::endl;
    else
        std::cout << IO::toString(res.back());
*/
/* Testing straightenSnake */
/*
    auto res = straightenSnake(init);
    std::cout << IO::toString(res.back());
*/
/* Testing fixParity */
/*
    auto res = fixParity(init);
    if (!res.empty())
        std::cout << IO::toString(res.back());
*/
/* Testing fixDocks */
/*
    auto res = fixDocks(init);
    if (!res.empty())
        std::cout << IO::toString(res.back());
*/
/* Testing flattenCircle */
/*
    auto res = flattenCircle(init);
    if (!res.empty())
        std::cout << IO::toString(res.back());
*/
/* Testing ALL */
/*
    auto res = reconfigToSnake(init);
    if (!res.empty())
        std::cout << IO::toString(res.back());
    std::cout << "Path len: " << res.size() << std::endl;
    if (argc > 2)
        printVecToFile(res, argv[2]);
*/
/* Testing really all */
/*
    std::ifstream goalInput;
    initInput.open(argv[2]);
    Configuration goal;
    IO::readConfiguration(goalInput, goal);
    goal.computeMatrices();
    auto res = reconfigThroughSnake(init, goal);
    printVecToFile(res, argv[3]);
*/
/* Testing with path debug */
/*
    std::ofstream file;
    file.open(std::string(argv[2]), std::ios_base::app);
    file << argv[1] << std::endl;
    auto res = reconfigToSnake(init, &file).first;
    printVecToFile(res, argv[3]);
*/
/* Testing mapping */
/**/
    std::vector<Configuration> toSnake1;
    std::vector<Configuration> toSnake2;
    std::ifstream input1;
    input1.open(argv[1]);
    std::ifstream input2;
    input2.open(argv[2]);

    IO::readConfigurations(input1, toSnake1);
    IO::readConfigurations(input2, toSnake2);

    appendMapped(toSnake1, toSnake2);

    printVecToFile(toSnake1, argv[3]);

/**/
    return 0;
}