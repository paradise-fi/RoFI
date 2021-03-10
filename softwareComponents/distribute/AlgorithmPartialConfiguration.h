//
// Created by xmichel on 5/20/19.
//

#ifndef ROFI_ALGORITHMPARTIALCONFIGURATION_H
#define ROFI_ALGORITHMPARTIALCONFIGURATION_H


#include "DistributedModule.h"
#include <set>
#include <queue>

enum MPITags {
    shareModulePropTag,
    hasMatrixTag,
    shareMatrixTag,
    shareMatrixForConnectTag,
    canConnectTag,
    shareEdgeTag
};

enum ActionType {
    Rotation,
    Connection,
    Disconnection,
    NoAction
};

class AlgorithmPartialConfiguration : public DistributedModule {
public:
    AlgorithmPartialConfiguration(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream);

    bool reconfigurate();

private:
    Matrix matrixA;
    bool matrixAInit = false;

    Matrix matrixB;
    bool matrixBInit = false;

    void shareCoordinates();

    ID nextId(ID current) const;

    bool execAction(int indexOfAction, int step);

    void execAction(ID other);

    bool tryConnect(const Edge &edge, int step);

    void tryConnect(ID other);

    bool tryDisconnect(const Edge &edge, int step);

    void tryDisconnect(ID other);

    bool tryRotation(Joint joint, double angle, int step);

    void tryRotation(ID other);

    void tryRotationRotateModules(ID other, int rotateModulesCount);

    void tryRotationStaticModules(ID other, int rotateModulesCount) const;

    bool getIds(const int *neighboursId, ShoeId side, std::set<ID> &idsOnSide) const;

    void getNeighboursIds(int *otherNeighbours, ID root) const;

    void createCfg(const std::vector<DistributedModuleProperties> &neighbours, Configuration &cfg) const;

    bool existPath(const int *neighbours, ID id1, ID id2) const;
};

inline bool compareEdge(const Edge edge1, const Edge edge2) {
    return edge1.id2() < edge2.id2() || (edge1.id2() == edge2.id2() && edge1.side2() < edge2.side2());
}


#endif //ROFI_ALGORITHMPARTIALCONFIGURATION_H
