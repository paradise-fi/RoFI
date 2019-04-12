//
// Created by xmichel on 25.3.19.
//

#ifndef ROFI_DISTRIBUTEDMODULEPROPERTIES_H
#define ROFI_DISTRIBUTEDMODULEPROPERTIES_H

#include "../Configuration.h"

class DistributedModuleProperties : public Module {
public:
    DistributedModuleProperties() : Module(0, 0, 0, static_cast<ID>(0)) {}

    DistributedModuleProperties(double alpha, double beta, double gamma, ID id) : Module(alpha, beta, gamma, id) {}

    EdgeList getEdges() const {
        return edges;
    }

    void setEdges(const EdgeList &newEdges) {
        edges = newEdges;
    }

private:
    EdgeList edges;
};

#endif //ROFI_DISTRIBUTEDMODULEPROPERTIES_H
