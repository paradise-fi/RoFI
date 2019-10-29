//
// Created by xmichel on 5/20/19.
//

#include "AlgorithmPartialConfiguration.h"

AlgorithmPartialConfiguration::AlgorithmPartialConfiguration(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream)
        : DistributedModule(id, inStream, trgStream) {

}

bool AlgorithmPartialConfiguration::reconfigurate() {
    if (currModule.getId() == 0) {
        matrixA = identity;
        matrixAInit = true;
    }

    shareCoordinates();

    ID lastActionID = 0;
    int indexOfAction = 0;
    ID currentID = 0;
    int step = 1;
    bool endOfReconfig = false;
    bool firstRound = true;
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    while (!endOfReconfig) {
        if (indexOfAction >= 6 * 2 + 3) {
            //no step is available - TODO try heuristic
            return false;
        }

        bool executed = false;
        if (currentID == currModule.getId()) {
            executed = execAction(indexOfAction, step);
        } else {
            execAction(currentID);
        }
        MPI_Bcast(&executed, 1, MPI_CXX_BOOL, currentID, MPI_COMM_WORLD);

        if (executed) {
            lastActionID = currentID;
            indexOfAction = 0;
            step++;
        } else if (currentID == lastActionID && !firstRound) {
            indexOfAction++;
        }

        Action action = currModule.diff(trgModule);
        bool needToReconfig = !action.empty();
        bool needToReconfigSend[worldSize];
        for (int i = 0; i < worldSize; i++) {
            needToReconfigSend[i] = needToReconfig;
        }
        bool needToReconfigRecv[worldSize];

        MPI_Alltoall(needToReconfigSend, 1, MPI_CXX_BOOL, needToReconfigRecv, 1, MPI_CXX_BOOL
                , MPI_COMM_WORLD);

        currentID = nextId(currentID);
        bool someNeedReconfig = false;
        for (int i = 0; i < worldSize; i++) {
            someNeedReconfig |= needToReconfigRecv[i];
        }
        endOfReconfig = !someNeedReconfig;
        firstRound = false;
    }

    return true;
}

void AlgorithmPartialConfiguration::shareCoordinates() {
    std::vector<Edge> edges;
    for (auto &edge : currModule.getEdges()) {
        if (edge.has_value()) {
            edges.push_back(edge.value());
        }
    }
    std::sort(edges.begin(), edges.end(), compareEdge);

    std::vector<DistributedModuleProperties> neighbours;
    for (auto &edge : edges) {
        MPI_Status status;
        auto currModuleToSend = reinterpret_cast<const char *>(&currModule);
        char moduleToRecv[sizeof(DistributedModuleProperties)];
        MPI_Sendrecv(currModuleToSend, sizeof(DistributedModuleProperties), MPI_CHAR, edge.id2(), shareModulePropTag
                , moduleToRecv, sizeof(DistributedModuleProperties), MPI_CHAR, edge.id2(), shareModulePropTag
                , MPI_COMM_WORLD, &status);
        neighbours.push_back(*reinterpret_cast<DistributedModuleProperties *>(moduleToRecv));
    }

    Configuration cfg;
    createCfg(neighbours, cfg);

    std::vector<ID> idsHasMatrix;
    while (!matrixAInit && !matrixBInit) {
        for (const auto &edge : edges) {
            MPI_Status status;
            bool toSend = false;
            bool toRecv;
            MPI_Sendrecv(&toSend, 1, MPI_CXX_BOOL, edge.id2(), hasMatrixTag
                    , &toRecv, 1, MPI_CXX_BOOL, edge.id2(), hasMatrixTag
                    , MPI_COMM_WORLD, &status);

            if (toRecv) {
                char matrixToRecv[sizeof(Matrix)];
                MPI_Recv(matrixToRecv, sizeof(Matrix), MPI_CHAR, edge.id2(), shareMatrixTag, MPI_COMM_WORLD, &status);
                auto matrix = reinterpret_cast<Matrix *>(matrixToRecv);

                if (edge.side1() == A) {
                    matrixA = *matrix;
                    matrixAInit = true;
                } else {
                    matrixB = *matrix;
                    matrixBInit = true;
                }

                idsHasMatrix.push_back(edge.id2());
            }
        }
    }

    if (matrixAInit) {
        cfg.setFixed(currModule.getId(), A, matrixA);
    } else {
        cfg.setFixed(currModule.getId(), B, matrixB);
    }

    cfg.computeMatrices();
    const MatrixMap matrices = cfg.getMatrices();

    if (matrixAInit) {
        matrixB = matrices.at(currModule.getId()).at(1);
        matrixBInit = true;
    } else {
        matrixA = matrices.at(currModule.getId()).at(0);
        matrixAInit = true;
    }

    for (const auto &edge : edges) {
        bool hasMatrix = false;
        for (ID &id : idsHasMatrix) {
            if (edge.id2() == id) {
                hasMatrix = true;
                break;
            }
        }

        if (hasMatrix) {
            continue;
        }

        MPI_Status status;
        bool toSend = true;
        bool toRecv;
        MPI_Sendrecv(&toSend, 1, MPI_CXX_BOOL, edge.id2(), hasMatrixTag
                , &toRecv, 1, MPI_CXX_BOOL, edge.id2(), hasMatrixTag
                , MPI_COMM_WORLD, &status);

        if (!toRecv) {
            Matrix matrix = matrices.at(edge.id2()).at(edge.side2());
            auto matrixToSend = reinterpret_cast<const char *>(&matrix);
            MPI_Send(matrixToSend, sizeof(Matrix), MPI_CHAR, edge.id2(), shareMatrixTag, MPI_COMM_WORLD);
        }
    }
}

ID AlgorithmPartialConfiguration::nextId(ID current) const {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    current++;
    current %= worldSize;

    return current;
}

bool AlgorithmPartialConfiguration::execAction(int indexOfAction, int step) {
    Action action = currModule.diff(trgModule);
    if (action.rotations().size() > indexOfAction) {
        ActionType actionType = ActionType::Rotation;
        MPI_Bcast(&actionType, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);
        Action::Rotate rotation = action.rotations().at(indexOfAction);
        return tryRotation(rotation.joint(), rotation.angle(), step);
    }

    indexOfAction -= action.rotations().size();

    if (action.reconnections().size() > indexOfAction) {
        Action::Reconnect reconnect = action.reconnections().at(indexOfAction);
        if (reconnect.add()) {
            ActionType actionType = ActionType::Connection;
            MPI_Bcast(&actionType, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);
            return tryConnect(reconnect.edge(), step);
        }

        ActionType actionType = ActionType::Disconnection;
        MPI_Bcast(&actionType, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);
        return tryDisconnect(reconnect.edge(), step);
    }

    ActionType actionType = ActionType::NoAction;
    MPI_Bcast(&actionType, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);
    return false;
}

void AlgorithmPartialConfiguration::execAction(ID other) {
    ActionType actionType;
    MPI_Bcast(&actionType, 1, MPI_INT, other, MPI_COMM_WORLD);

    switch (actionType) {
        case Rotation:
            tryRotation(other);
            break;
        case Connection:
            tryConnect(other);
            break;
        case Disconnection:
            tryDisconnect(other);
            break;
        case NoAction:
            break;
    }
}

bool AlgorithmPartialConfiguration::tryConnect(const Edge &edge, int step) {
    int id2 = edge.id2();
    MPI_Bcast(&id2, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);

    Matrix matrix;
    if (edge.side1() == ShoeId::A) {
        matrix = matrixA * transformConnection(edge.dock1(), edge.ori(), edge.dock2());
    } else {
        matrix = matrixB * transformConnection(edge.dock1(), edge.ori(), edge.dock2());
    }

    std::pair<Matrix, Edge> pairToSend = {matrix, edge};
    auto pairToSendChar = reinterpret_cast<const char *>(&pairToSend);
    MPI_Send(pairToSendChar, sizeof(std::pair<Matrix, Edge>), MPI_CHAR, id2, shareMatrixForConnectTag, MPI_COMM_WORLD);

    MPI_Status status;
    bool canConnect;
    MPI_Recv(&canConnect, 1, MPI_CXX_BOOL, id2, canConnectTag, MPI_COMM_WORLD, &status);

    if (canConnect) {
        currModule.addEdge(edge);
        Action::Reconnect reconnect(true, edge);
        std::cout << DistributedPrinter::toString(reconnect, step);
        return true;
    }

    return false;
}

void AlgorithmPartialConfiguration::tryConnect(ID other) {
    int id;
    MPI_Bcast(&id, 1, MPI_INT, other, MPI_COMM_WORLD);

    if (id != currModule.getId()) {
        return;
    }

    char pairToRecvChar[sizeof(std::pair<Matrix, Edge>)];
    MPI_Status status;
    MPI_Recv(pairToRecvChar, sizeof(std::pair<Matrix, Edge>), MPI_CHAR, other, shareMatrixForConnectTag, MPI_COMM_WORLD, &status);
    auto [matrix, edge] = *reinterpret_cast<std::pair<Matrix, Edge> *>(pairToRecvChar);

    bool canConnect = (edge.side2() == ShoeId::A && equals(matrixA, matrix))
                      || (edge.side2() == ShoeId::B && equals(matrixB, matrix));
    MPI_Send(&canConnect, 1, MPI_CXX_BOOL, other, canConnectTag, MPI_COMM_WORLD);

    if (canConnect) {
        currModule.addEdge(edge);
    }
}

bool AlgorithmPartialConfiguration::tryDisconnect(const Edge &edge, int step) {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int otherNeighbours[6 * worldSize];
    getNeighboursIds(otherNeighbours, currModule.getId());

    otherNeighbours[edge.id2() * 6 + edge.side2() * 3 + edge.dock2()] = -1;
    otherNeighbours[edge.id1() * 6 + edge.side1() * 3 + edge.dock1()] = -1;

    bool canDisconnect = existPath(otherNeighbours, edge.id1(), edge.id2());
    MPI_Bcast(&canDisconnect, 1, MPI_CXX_BOOL, currModule.getId(), MPI_COMM_WORLD);

    if (!canDisconnect) {
        return false;
    }

    int id2 = edge.id2();
    MPI_Bcast(&id2, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);

    auto edgeToSend = reinterpret_cast<const char *>(&edge);
    MPI_Send(edgeToSend, sizeof(Edge), MPI_CHAR, id2, shareEdgeTag, MPI_COMM_WORLD);

    currModule.removeEdge(edge);
    Action::Reconnect reconnect(false, edge);
    std::cout << DistributedPrinter::toString(reconnect, step);

    return true;
}

void AlgorithmPartialConfiguration::tryDisconnect(ID other) {
    getNeighboursIds(nullptr, other);

    bool canDisconnect;
    MPI_Bcast(&canDisconnect, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);

    if (!canDisconnect) {
        return;
    }

    int id2;
    MPI_Bcast(&id2, 1, MPI_INT, other, MPI_COMM_WORLD);

    if (id2 != currModule.getId()) {
        return;
    }

    MPI_Status status;
    char edgeChar[sizeof(Edge)];
    MPI_Recv(edgeChar, sizeof(Edge), MPI_CHAR, other, shareEdgeTag, MPI_COMM_WORLD, &status);

    Edge edge = *reinterpret_cast<Edge *>(edgeChar);
    currModule.removeEdge(edge);
}

bool AlgorithmPartialConfiguration::tryRotation(Joint joint, double angle, int step) {
    int worldSize;
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    int otherNeighbours[6 * worldSize];
    getNeighboursIds(otherNeighbours, currModule.getId());

    std::set<ID> idsOnSide;
    bool canMove = getIds(otherNeighbours, joint == Joint::Beta ? B : A, idsOnSide);
    MPI_Bcast(&canMove, 1, MPI_CXX_BOOL, currModule.getId(), MPI_COMM_WORLD);

    if (!canMove) {
        return false;
    }

    int idCount = idsOnSide.size();
    MPI_Bcast(&idCount, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);

    int ids[idCount];
    int counter = 0;
    for (ID id : idsOnSide) {
        ids[counter] = id;
        counter++;
    }
    MPI_Bcast(ids, idCount, MPI_INT, currModule.getId(), MPI_COMM_WORLD);

    std::vector<DistributedModuleProperties> otherModules;
    for (int i = 0; i < idCount; i++) {
        MPI_Status status;
        char moduleToRecv[sizeof(DistributedModuleProperties)];
        MPI_Recv(moduleToRecv, sizeof(DistributedModuleProperties), MPI_CHAR, ids[i], shareModulePropTag
                , MPI_COMM_WORLD, &status);

        otherModules.push_back(*reinterpret_cast<DistributedModuleProperties *>(moduleToRecv));
    }

    Configuration cfg;
    createCfg(otherModules, cfg);
    if (joint == Joint::Beta) {
        cfg.setFixed(currModule.getId(), B, matrixB);
    } else {
        cfg.setFixed(currModule.getId(), A, matrixA);
    }
    cfg.computeMatrices();

    Action::Rotate rotation(currModule.getId(), joint, angle);
    Action action = { { rotation }, { } };
    int rotationSteps = 10;
    MPI_Bcast(&rotationSteps, 1, MPI_INT, currModule.getId(), MPI_COMM_WORLD);

    Action divided = action.divide(1.0/rotationSteps);
    bool canNextStep = true;
    for (int i = 0; i < rotationSteps && canNextStep; i++) {
        cfg.execute(divided);
        cfg.computeMatrices();
        MatrixMap matrices = cfg.getMatrices();
        std::pair<ID, std::array<Matrix, 2>> matricesToSend[idCount];
        counter = 0;
        for (auto &[id, array] : matrices) {
            if (id == currModule.getId()) {
                continue;
            }

            matricesToSend[counter] = {id, array};
            counter++;
        }

        auto matricesToSendChar = reinterpret_cast<char *>(matricesToSend);
        MPI_Bcast(matricesToSendChar, sizeof(std::pair<ID, std::array<Matrix, 2>>) * idCount, MPI_CHAR
                , currModule.getId(), MPI_COMM_WORLD);

        bool collisionFree[idCount];
        bool collFreeSend = true;
        MPI_Gather(&collFreeSend, 1, MPI_CXX_BOOL, collisionFree, 1, MPI_CXX_BOOL, currModule.getId(), MPI_COMM_WORLD);

        canNextStep = true;
        for (bool oneModuleFree : collisionFree) {
            canNextStep &= oneModuleFree;
        }

        MPI_Bcast(&canNextStep, 1, MPI_CXX_BOOL, currModule.getId(), MPI_COMM_WORLD);
    }

    if (canNextStep) {
        cfg.computeMatrices();
        MatrixMap matrices = cfg.getMatrices();
        matrixA = matrices.at(currModule.getId())[0];
        matrixB = matrices.at(currModule.getId())[1];
        currModule.rotateJoint(joint, angle);

        std::cout << DistributedPrinter::toString(rotation, step);
        return true;
    }

    return false;
}

void AlgorithmPartialConfiguration::tryRotation(ID other) {
    getNeighboursIds(nullptr, other);

    bool canMove;
    MPI_Bcast(&canMove, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);

    if (!canMove) {
        return;
    }

    int idCount;
    MPI_Bcast(&idCount, 1, MPI_INT, other, MPI_COMM_WORLD);

    int ids[idCount];
    MPI_Bcast(ids, idCount, MPI_INT, other, MPI_COMM_WORLD);

    bool shouldSentProp = false;
    for (int i = 0; i < idCount; i++) {
        if (ids[i] == currModule.getId()) {
            shouldSentProp = true;
        }
    }

    if (shouldSentProp) {
        tryRotationRotateModules(other, idCount);
    } else {
        tryRotationStaticModules(other, idCount);
    }
}

void AlgorithmPartialConfiguration::tryRotationRotateModules(ID other, int rotateModulesCount) {
    auto currModuleToSend = reinterpret_cast<const char *>(&currModule);
    MPI_Send(currModuleToSend, sizeof(DistributedModuleProperties), MPI_CHAR, other, shareModulePropTag
            , MPI_COMM_WORLD);

    int rotationSteps;
    MPI_Bcast(&rotationSteps, 1, MPI_INT, other, MPI_COMM_WORLD);

    bool canNextStep = true;
    std::array<Matrix, 2> currModuleMatrices;
    for (int i = 0; i < rotationSteps && canNextStep; i++) {
        char matricesToRecvChar[rotateModulesCount * sizeof(std::pair<ID, std::array<Matrix, 2>>)];
        MPI_Bcast(matricesToRecvChar, sizeof(std::pair<ID, std::array<Matrix, 2>>) * rotateModulesCount, MPI_CHAR, other
                , MPI_COMM_WORLD);

        bool collFreeSend = true;
        MPI_Gather(&collFreeSend, 1, MPI_CXX_BOOL, nullptr, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);
        MPI_Bcast(&canNextStep, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);

        if (i == rotationSteps - 1 && canNextStep) {
            auto matricesToRecv = reinterpret_cast<std::pair<ID, std::array<Matrix, 2>> *>(matricesToRecvChar);
            for (int j = 0; j < rotateModulesCount && collFreeSend; j++) {
                std::pair<ID, std::array<Matrix, 2>> &pair = matricesToRecv[j];
                if (currModule.getId() == std::get<0>(pair)) {
                    currModuleMatrices = std::get<1>(pair);
                    break;
                }
            }
        }
    }

    if (canNextStep) {
        matrixA = currModuleMatrices[0];
        matrixB = currModuleMatrices[1];
    }
}

void AlgorithmPartialConfiguration::tryRotationStaticModules(ID other, int rotateModulesCount) const {
    int rotationSteps;
    MPI_Bcast(&rotationSteps, 1, MPI_INT, other, MPI_COMM_WORLD);

    bool canNextStep = true;
    for (int i = 0; i < rotationSteps && canNextStep; i++) {
        char matricesToRecvChar[rotateModulesCount * sizeof(std::pair<ID, std::array<Matrix, 2>>)];
        MPI_Bcast(matricesToRecvChar, sizeof(std::pair<ID, std::array<Matrix, 2>>) * rotateModulesCount, MPI_CHAR, other
                , MPI_COMM_WORLD);

        bool collFreeSend = true;
        auto matricesToRecv = reinterpret_cast<std::pair<ID, std::array<Matrix, 2>> *>(matricesToRecvChar);
        for (int j = 0; j < rotateModulesCount && collFreeSend; j++) {
            std::pair<ID, std::array<Matrix, 2>> &pair = matricesToRecv[j];
            collFreeSend &= distance(std::get<1>(pair)[0], matrixA) >= 1;
            collFreeSend &= distance(std::get<1>(pair)[0], matrixB) >= 1;
            collFreeSend &= distance(std::get<1>(pair)[1], matrixA) >= 1;
            collFreeSend &= distance(std::get<1>(pair)[1], matrixB) >= 1;
        }

        MPI_Gather(&collFreeSend, 1, MPI_CXX_BOOL, nullptr, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);
        MPI_Bcast(&canNextStep, 1, MPI_CXX_BOOL, other, MPI_COMM_WORLD);
    }
}

void AlgorithmPartialConfiguration::getNeighboursIds(int *otherNeighbours, ID root) const {
    int neighbourIds[6];
    for (unsigned long i = 0; i < 6; i++) {
        if (currModule.getEdges().at(i).has_value()) {
            neighbourIds[i] = currModule.getEdges().at(i).value().id2();
        } else {
            neighbourIds[i] = -1;
        }
    }

    MPI_Gather(neighbourIds, 6, MPI_INT, otherNeighbours, 6, MPI_INT, root, MPI_COMM_WORLD);
}

bool AlgorithmPartialConfiguration::getIds(const int *neighboursId, ShoeId side, std::set<ID> &idsOnSide) const {
    ShoeId otherSide = side == A ? B : A;

    std::set<ID> otherSideIds;
    for (int i = currModule.getId() * 6 + 3 * otherSide; i < currModule.getId() * 6 + 3 * otherSide + 3; i++) {
        if (neighboursId[i] == -1) {
            continue;
        }

        otherSideIds.insert(neighboursId[i]);
    }

    std::queue<ID> idsToProcess;
    for (int i = currModule.getId() * 6 + 3 * side; i < currModule.getId() * 6 + 3 * side + 3; i++) {
        if (neighboursId[i] == -1) {
            continue;
        }

        if (otherSideIds.find(neighboursId[i]) != otherSideIds.end()) {
            return false;
        }

        idsToProcess.push(neighboursId[i]);
    }

    while (!idsToProcess.empty()) {
        ID id = idsToProcess.front();
        idsToProcess.pop();

        if (idsOnSide.find(id) != idsOnSide.end() || id == currModule.getId()) {
            continue;
        }

        if (otherSideIds.find(id) != otherSideIds.end()) {
            return false;
        }

        idsOnSide.insert(id);

        for (int i = id * 6; i < id * 6 + 6; i++) {
            if (neighboursId[i] == -1 || idsOnSide.find(neighboursId[i]) != idsOnSide.end()) {
                continue;
            }

            idsToProcess.push(neighboursId[i]);
        }
    }

    return true;
}

void AlgorithmPartialConfiguration::createCfg(const std::vector<DistributedModuleProperties> &neighbours, Configuration &cfg) const {
    cfg.addModule(currModule.getJoint(Joint::Alpha), currModule.getJoint(Joint::Beta), currModule.getJoint(Joint::Gamma)
            , currModule.getId());

    for (auto &module : neighbours) {
        cfg.addModule(module.getJoint(Joint::Alpha), module.getJoint(Joint::Beta), module.getJoint(Joint::Gamma)
                , module.getId());
    }

    const ModuleMap &modules = cfg.getModules();
    for (auto &module : neighbours) {
        for (auto optEdge : module.getEdges()) {
            if (!optEdge.has_value()) {
                continue;
            }

            Edge edge = optEdge.value();
            ID id1 = edge.id1();
            ID id2 = edge.id2();

            if (module.getId() == std::min(id1, id2) && modules.find(std::max(id1, id2)) != modules.end()) {
                cfg.addEdge(edge);
            }
        }
    }

    for (auto optEdge : currModule.getEdges()) {
        if (!optEdge.has_value()) {
            continue;
        }

        Edge edge = optEdge.value();
        ID id1 = edge.id1();
        ID id2 = edge.id2();

        if (currModule.getId() == std::min(id1, id2) && modules.find(std::max(id1, id2)) != modules.end()) {
            cfg.addEdge(edge);
        }
    }
}

bool AlgorithmPartialConfiguration::existPath(const int *neighbours, ID id1, ID id2) const {
    std::queue<ID> queue;
    queue.push(id1);

    std::set<ID> seenIds;
    while (!queue.empty()) {
        ID id = queue.front();
        queue.pop();

        if (id == id2) {
            return true;
        }

        if (seenIds.find(id) != seenIds.end()) {
            continue;
        }

        for (int i = id * 6; i < id * 6 + 6; i++) {
            if (neighbours[i] != -1) {
                queue.push(neighbours[i]);
            }
        }

        seenIds.insert(id);
    }

    return false;
}
