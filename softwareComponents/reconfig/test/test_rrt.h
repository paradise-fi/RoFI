#ifndef ROBOTS_TEST_RRT_H
#define ROBOTS_TEST_RRT_H

std::optional<Edge> generateEdge(ID id1, ID id2, std::unordered_map<ID, std::array<bool, 6>>& occupied);
std::optional<Configuration> generateAngles(const std::vector<ID>& ids, const std::vector<Edge>& edges);

#endif //ROBOTS_TEST_RRT_H
