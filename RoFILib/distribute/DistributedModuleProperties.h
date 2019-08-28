//
// Created by xmichel on 25.3.19.
//

#ifndef ROFI_DISTRIBUTEDMODULEPROPERTIES_H
#define ROFI_DISTRIBUTEDMODULEPROPERTIES_H

#include <Configuration.h>

class DistributedModuleProperties : public Module {
public:
    DistributedModuleProperties() : Module(0, 0, 0, static_cast<ID>(0)) {}

    explicit DistributedModuleProperties(ID id) : Module(0, 0, 0, static_cast<ID>(id)) {}

    DistributedModuleProperties(double alpha, double beta, double gamma, ID id) : Module(alpha, beta, gamma, id) {}

    void addEdge(const Edge &edge) {
        if (edge.id1() == getId()) {
            edges[edge.side1() * 3 + edge.dock1()] = edge;
        } else if (edge.id2() == getId()) {
            edges[edge.side2() * 3 + edge.dock2()] = reverse(edge);
        }
    }

    void removeEdge(const Edge &edge) {
        if (edge.id1() == getId()) {
            edges[edge.side1() * 3 + edge.dock1()] = std::nullopt;
        } else if (edge.id2() == getId()) {
            edges[edge.side2() * 3 + edge.dock2()] = std::nullopt;
        }
    }

    void changeEdge(const std::optional<Edge> edge, unsigned long i) {
        if (edge.has_value()) {
            if (edge.value().id1() == getId()) {
                edges[i] = edge;
            } else {
                edges[i] = reverse(edge.value());
            }
        } else {
            edges[i] = std::nullopt;
        }
    }

    EdgeList getEdges() const {
        return edges;
    }

    void setEdges(const EdgeList &newEdges) {
        for (unsigned long i = 0; i < 6; i++) {
            changeEdge(newEdges.at(i), i);
        }
    }

    void execute(const Action::Rotate &rotation) {
        if (rotation.id() == getId()) {
            rotateJoint(rotation.joint(), rotation.angle());
        }
    }

    void execute(const Action::Reconnect &reconnection) {
        unsigned long index;
        if (reconnection.edge().id1() == getId()) {
            index = reconnection.edge().side1() * 3 + reconnection.edge().dock1();
        } else if (reconnection.edge().id2() == getId()) {
            index = reconnection.edge().side2() * 3 + reconnection.edge().dock2();
        } else {
            return;
        }

        if (reconnection.add()) {
            changeEdge(reconnection.edge(), index);
        } else {
            edges[index] = std::nullopt;
        }
    }

    Action diff(const DistributedModuleProperties &other) const {
        std::vector<Action::Rotate> rotations;
        for (Joint joint : { Alpha, Beta, Gamma }) {
            if (getJoint(joint) != other.getJoint(joint)) {
                rotations.emplace_back(getId(), joint, other.getJoint(joint) - getJoint(joint));
            }
        }

        std::vector<Action::Reconnect> reconnections;
        for (unsigned long i = 0; i < 6; i++) {
            if (edges.at(i).has_value() && edges.at(i) != other.edges.at(i)) {
                reconnections.emplace_back(false, edges.at(i).value());
            }
        }

        for (unsigned long i = 0; i < 6; i++) {
            if (other.edges.at(i).has_value() && edges.at(i) != other.edges.at(i)) {
                reconnections.emplace_back(true, other.edges.at(i).value());
            }
        }

        return {rotations, reconnections};
    }

private:
    EdgeList edges;
};

#endif //ROFI_DISTRIBUTEDMODULEPROPERTIES_H
