#include "Algorithms.h"
#include "test/test_rrt.h"

namespace Distance
{
    inline double reconnections(const Configuration& curr, const Configuration& goal) {
        auto diff = curr.diff(goal);
        return double(diff.reconnections().size());
    }

    inline double rotations(const Configuration& curr, const Configuration& goal) {
        auto diff = curr.diff(goal);
        return double(diff.rotations().size());
    }

    inline double diff(const Configuration& curr, const Configuration& goal) {
        return reconnections(curr, goal) * 1000 + rotations(curr, goal);
    }
}

inline int findFreeIndex(int index, const std::array<bool, 6>& A)
{
    int count = 6;
    while (!A[index] && count > 0)
    {
        index = (index + 1) % 6;
        --count;
    }
    if (count == 0)
    {
        return -1;
    }
    return index;
}

std::optional<Edge> generateEdge(ID id1, ID id2, std::unordered_map<ID, std::array<bool, 6>>& occupied)
{
    std::default_random_engine rd;
    rd.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> dist(0, 11);

    int index1 = findFreeIndex(dist(rd) % 6, occupied[id1]);
    int index2 = findFreeIndex(dist(rd) % 6, occupied[id2]);

    occupied[id1][index1] = false;
    occupied[id2][index2] = false;

    if ((index1 < 0) || (index2 < 0))
    {
        return std::nullopt;
    }
    int ori = dist(rd) % 4;
    auto s1 = ShoeId(index1 % 3 % 2);
    auto d1 = ConnectorId(index1 / 3);
    auto s2 = ShoeId(index2 % 3 % 2);
    auto d2 = ConnectorId(index2 / 3);
    return Edge(id1, s1, d1, ori, d2, s2, id2);
}

std::optional<Configuration> generateAngles(const std::vector<ID>& ids, const std::vector<Edge>& edges)
{
    // Assume the edges form a connected graph and that edges contain only IDs from the 'ids' vector.
    std::default_random_engine rd;
    rd.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int> ab(-1,1);
    std::uniform_int_distribution<int> c(-1, 2);
    double step = 90;
    Configuration cfg;
    for (ID id : ids)
    {
        double alpha = step * ab(rd);
        double beta = step * ab(rd);
        double gamma = step * c(rd);
        cfg.addModule(alpha,beta,gamma,id);
    }
    for (const Edge& edge : edges)
    {
        cfg.addEdge(edge);
    }

    // Try generating random angles to make the configuration valid.
    // After 10 iterations give up.
    for (int i = 0; i < 10; ++i)
    {
        if (cfg.isValid())
            return cfg;
        for (auto& [id, mod] : cfg.getModules())
        {
            double alpha = step * ab(rd);
            double beta = step * ab(rd);
            double gamma = step * c(rd);
            mod.setJoint(Joint::Alpha, alpha);
            mod.setJoint(Joint::Beta, beta);
            mod.setJoint(Joint::Gamma, gamma);
        }
    }
    // Giving up.
    return std::nullopt;
}

inline std::optional<Configuration> generateRandomTree(const std::vector<ID>& ids)
{
    std::default_random_engine rd;
    rd.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<unsigned long> dist(0, ids.size());


    std::vector<Edge> edges;
    std::vector<ID> shuffled = ids;
    std::shuffle(shuffled.begin(), shuffled.end(), rd);
    std::unordered_map<ID, std::array<bool, 6>> occupied;
    occupied[shuffled[0]] = {true, true, true, true, true, true};

    for (unsigned i = 1; i < shuffled.size(); ++i)
    {
        ID id1 = shuffled[i];
        ID id2 = shuffled[dist(rd) % i];
        occupied[id1] = {true, true, true, true, true, true};
        std::optional<Edge> edgeOpt = generateEdge(id1, id2, occupied);
        if (!edgeOpt.has_value())
        {
            return std::nullopt;
        }

        edges.emplace_back(edgeOpt.value());
    }
    return generateAngles(ids, edges);
}

inline Configuration sampleFree(const std::vector<ID>& ids)
{
    auto treeOpt = generateRandomTree(ids);
    while (!treeOpt.has_value())
    {
        treeOpt = generateRandomTree(ids);
    }
    auto cfg = treeOpt.value();
    std::vector<Action::Reconnect> connect;
    generateConnections(cfg, connect);

    std::default_random_engine rd;
    rd.seed(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<unsigned long> dist(0, 1);
    for (auto& action : connect)
    {
        if (dist(rd) == 1)
        {
            cfg.execute({{},{action}});
        }
    }

    return cfg;
}

inline const Configuration* getCfg(const std::unique_ptr<Configuration>& ptr)
{
    return ptr.get();
}

inline const Configuration* getCfg(const Configuration& cfg)
{
    return &cfg;
}


inline const Configuration* addToTree(ConfigPool& pool, ConfigEdges& edges,
    const Configuration* from, const Configuration& to)
{
    if (!pool.has(to))
    {
        auto nextPtr = pool.insert(to);
        edges[nextPtr] = {};

        edges[from].push_back(nextPtr);
        edges[nextPtr].push_back(from);
        return nextPtr;
    }
    else
    {
        return pool.get(to);
    }
}

template<typename T>
inline const Configuration* nearest(const Configuration& cfg, const T& pool, DistFunction* dist)
{
    const Configuration* near = nullptr;
    double dMin = 0;
    for (auto& next : pool)
    {
        auto nextPtr = getCfg(next);
        if (near == nullptr)
        {
            near = nextPtr;
            dMin = dist(cfg, *near);
            continue;
        }
        double d = dist(cfg, *nextPtr);
        if (d < dMin)
        {
            dMin = d;
            near = nextPtr;
        }
    }
    return near;
}

inline Configuration steer(const Configuration& from, const Configuration& to, DistFunction* dist)
{
    auto diff = from.diff(to);

    auto init = executeIfValid(from, diff);
    if (init.has_value())
        return init.value();

    auto allRot = diff.rotations();
    auto allRec = diff.reconnections();
    std::vector<std::vector<Action::Rotate>> subRot;
    std::vector<std::vector<Action::Reconnect>> subRec;
    getAllSubsets(allRot, subRot, allRot.size());
    getAllSubsets(allRec, subRec, allRec.size());

    double minDistance = dist(from, to);
    Configuration minCfg = from;

    for (auto& rot : subRot)
    {
        for (auto& rec : subRec)
        {
            auto valid = executeIfValid(from, {rot, rec});
            if (valid.has_value())
            {
                double newDistance = dist(from, valid.value());
                if (newDistance < minDistance)
                {
                    minDistance = newDistance;
                    minCfg = valid.value();
                }
            }
        }
    }
    return minCfg;
}

inline Configuration steerRotate(const Configuration& from, const Configuration& /*to*/, const Action& diff)
{
    unsigned long count = diff.rotations().size();


    for (unsigned long i = count; i > 0; --i)
    {
        std::vector<std::vector<Action::Rotate>> subRot;
        std::vector<std::vector<Action::Rotate>> filteredRot;
        getAllSubsets(diff.rotations(), subRot, count);
        filter(subRot, filteredRot, unique);
        for (auto& rot : filteredRot)
        {
            Action action = {rot, {}};
            auto afterAction = executeIfValid(from, action);
            if (afterAction.has_value())
            {
                return afterAction.value();
            }
        }
    }
    return from;
}

inline std::vector<Configuration> steerReconnect(const Configuration& from,
    const Configuration& /*to*/, const Action& diff, unsigned /*step*/)
{
    auto allRec = diff.reconnections();
    unsigned long count = allRec.size();
    for (unsigned long i = count; i > 0; --i)
    {
        std::vector<std::vector<Action::Reconnect>> subRec;
        getAllSubsets(allRec, subRec, count);
        for (auto& rec : subRec)
        {
            Action action = {{}, rec};
            auto copy = from;
            copy.execute(action);
            std::vector<Edge> edges;
            auto ids = copy.getIDs();
            for (ID id : ids)
            {
                auto newEdges = copy.getEdges(id);
                edges.insert(edges.end(), newEdges.begin(), newEdges.end());
            }
            auto steered = generateAngles(ids, edges);
            if (!steered.has_value())
                continue;
            Action fromSteerDiff = from.diff(steered.value());
            Action fromSteerRot = {{fromSteerDiff.rotations()}, {}};
            auto first = executeIfValid(from, fromSteerRot);
            if (!first.has_value())
                continue;
            Action fromSteerRec = {{}, {fromSteerDiff.reconnections()}};
            auto second = executeIfValid(first.value(), fromSteerDiff);
            if (!second.has_value())
                continue;
            return {first.value(), second.value()};
        }
    }
    return {from};
}

inline std::vector<Configuration> steerPath(const Configuration& from, const Configuration& to, unsigned step)
{
    auto fromToDiff = from.diff(to);
    auto afterDiff = executeIfValid(from, fromToDiff);
    if (afterDiff.has_value())
    {
        return {afterDiff.value()};
    }
    if (fromToDiff.reconnections().empty())
    {
        return {steerRotate(from, to, fromToDiff)};
    }
    return steerReconnect(from, to, fromToDiff, step);
}

inline std::optional<Configuration> steerEdge(const Configuration& from, const Configuration& to, unsigned /*step*/)
{
    auto diff = from.diff(to);
    std::vector<Edge> edges;
    auto ids = from.getIDs();
    for (auto& rec : diff.reconnections())
    {
        if (rec.add())
            edges.push_back(rec.edge());
    }
    for (auto id : ids)
    {
        for (auto& edge : from.getEdges(id))
        {
            if (edge.id1() == id)
            {
                edges.push_back(edge);
            }
        }
    }
    return generateAngles(ids, edges);
}

inline void extendEdge(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step)
{
    const Configuration* near = nearest(cfg, pool, Eval::matrixDiff);
    auto cfgEdge = steerEdge(*near, cfg, step);
    if (!cfgEdge.has_value())
        return;
    auto diff = near->diff(cfgEdge.value());
    auto rot = diff.rotations();
    auto rec = diff.reconnections();

    auto first = executeIfValid(*near, {rot,{}});
    if (first.has_value())
    {
        auto ptr = addToTree(pool, edges, near, first.value());
        addToTree(pool, edges, ptr, cfgEdge.value());
    }
}

inline void extendPath(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step,
    DistFunction* dist = Eval::matrixDiff)
{
    const Configuration* near = nearest(cfg, pool, dist);
    auto newPath = steerPath(*near, cfg, step);
    for (auto& newCfg : newPath)
    {
        near = addToTree(pool, edges, near, newCfg);
    }
}


inline void extend(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg)
{
    const Configuration* near = nearest(cfg, pool, Distance::reconnections);
    Configuration next = steer(*near, cfg, Distance::rotations);

    addToTree(pool, edges, near, next);
}

inline void extend2(ConfigPool& pool, ConfigEdges& edges, const Configuration& cfg, unsigned step)
{
    const Configuration* near = nearest(cfg, pool, Distance::reconnections);

    auto diff = near->diff(cfg);
    auto value = executeIfValid(*near, diff);
    if (value.has_value())
    {
        addToTree(pool, edges, near, value.value());
        return;
    }

    auto cfgEdge = steerEdge(*near, cfg, step);
    if (cfgEdge.has_value())
    {
        auto diff2 = near->diff(cfgEdge.value());
        auto rot = diff2.rotations();

        auto first = executeIfValid(*near, {rot,{}});
        if (first.has_value())
        {
            auto ptr = addToTree(pool, edges, near, first.value());
            addToTree(pool, edges, ptr, cfgEdge.value());
        }
        return;
    }

    Configuration next = steer(*near, cfg, Distance::rotations);
    addToTree(pool, edges, near, next);
}

std::vector<Configuration> RRT(const Configuration& init, const Configuration& goal,
    unsigned step /*= 90*/, AlgorithmStat* stat /*= nullptr*/)
{
    ConfigPool pool;
    ConfigEdges edges;

    auto initPtr = pool.insert(init);

    while(true)
    {
        Configuration rand = sampleFree(init.getIDs());

        extendPath(pool, edges, rand, step);
        extendPath(pool, edges, goal, step);

        if (pool.has(goal))
            break;
    }

    stat->queueSize = 0;
    stat->seenCfgs = pool.size();
    if (pool.has(goal))
    {
        const Configuration * goalPtr = pool.get(goal);
        auto path = createPath(edges, initPtr, goalPtr);
        if (stat != nullptr)
        {
            stat->pathLength = path.size();
        }

        return path;
    }
    stat->pathLength = 0;
    return {};
}
