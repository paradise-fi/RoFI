//
// Created by xmichel on 25.3.19.
//

#ifndef ROFI_DISTRIBUTEDMODULE_H
#define ROFI_DISTRIBUTEDMODULE_H

#include "../Configuration.h"

class DistributedModule : public Module {
public:
    DistributedModule(double alpha, double beta, double gamma, ID id) : Module(alpha, beta, gamma, id) {}

    std::vector<Edge> getEdges() const {
        return edges;
    }

    void setEdges(const std::vector<Edge> &newEdges) {
        edges = newEdges;
    }

    void addEdge(Edge edge) {
        edges.push_back(edge);
    }

private:
    std::vector<Edge> edges;
};

#endif //ROFI_DISTRIBUTEDMODULE_H
