//
// Created by xmichel on 25.3.19.
//

#ifndef ROFI_DISTRIBUTEDMODULEPROPERTIES_H
#define ROFI_DISTRIBUTEDMODULEPROPERTIES_H

#include "../Configuration.h"

class DistributedModuleProperties : public Module {
public:
    DistributedModuleProperties() : Module(0, 0, 0, static_cast<ID>(0)) {}

    explicit DistributedModuleProperties(ID id) : Module(0, 0, 0, static_cast<ID>(id)) {}

    DistributedModuleProperties(double alpha, double beta, double gamma, ID id) : Module(alpha, beta, gamma, id) {}

    void addEdge(const Edge &edge) {
        if (edge.id1() == getId()) {
            edges[edge.side1() * 3 + edge.dock1()] = edge;
        } else if (edge.id2() == getId()) {
            edges[edge.side2() * 3 + edge.dock2()] = edge;
        }
    }

    EdgeList getEdges() const {
        return edges;
    }

    void setEdges(const EdgeList &newEdges) {
        edges = newEdges;
    }

    void execute(const Action::Rotate &rotation) {
        if (rotation.id() == getId()) {
            rotateJoint(rotation.joint(), rotation.angle());
        }
    }

    void execute(const Action::Reconnect &reconnection) {
        int index;
        if (reconnection.edge().id1() == getId()) {
            index = reconnection.edge().side1() * 3 + reconnection.edge().dock1();
        } else if (reconnection.edge().id2() == getId()) {
            index = reconnection.edge().side2() * 3 + reconnection.edge().dock2();
        } else {
            return;
        }

        if (reconnection.add()) {
            edges[index] = reconnection.edge();
        } else {
            edges[index] = std::nullopt;
        }
    }

private:
    EdgeList edges;
};

#endif //ROFI_DISTRIBUTEDMODULEPROPERTIES_H
