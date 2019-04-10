//
// Created by xmichel on 25.3.19.
//

#ifndef ROFI_DISTRIBUTEDMODULE_H
#define ROFI_DISTRIBUTEDMODULE_H

#include "../Configuration.h"

class DistributedModule : public Module {
public:
    DistributedModule() : Module(0, 0, 0, static_cast<ID>(0)) {}

    DistributedModule(double alpha, double beta, double gamma, ID id) : Module(alpha, beta, gamma, id) {}

    EdgeList getEdges() const {
        return edges;
    }

    void setEdges(const EdgeList &newEdges) {
        edges = newEdges;
    }

private:
    EdgeList edges;
};

#endif //ROFI_DISTRIBUTEDMODULE_H
