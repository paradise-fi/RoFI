#include "output.hpp"

using namespace arma;
using namespace rofi::torqueComputation;

std::string vecToString(const vec3& v) {
    std::stringstream stream;
    stream << "[";

    for (unsigned int i = 0; i < v.n_elem; i++) {
        if (i > 0) {
            stream << ", ";
        }
        stream << std::round(v(i) * 1000) / 1000;
    }

    stream << "]";
    return stream.str();
}

std::string vecToString(const std::vector<int>& v) {
    std::stringstream stream;
    stream << "[";

    for (unsigned int i = 0; i < v.size(); i++) {
        if (i > 0) {
            stream << ", ";
        }
        stream << v[i];
    }

    stream << "]";
    return stream.str();
}

std::string momentsToString(const mat33& moments) {
    std::stringstream stream;
    for (unsigned int row = 0; row < 3; row++) {
        stream << "            [";
        for (unsigned int col = 0; col < 3; col++) {
            if (col > 0) {
                stream << ", ";
            }
            stream << moments(row, col);
        }
        stream << "]";
        if (row < 2) {
            stream << ",";
        }
        stream << std::endl;
    }
    return stream.str();
}

void printJointsInfo(const std::unordered_map<int, std::unique_ptr<Joint>>& joints) {
    long unsigned int size = joints.size();
    long unsigned int i = 1;
    std::cout << "{\"joints\": {" << std::endl;

    for (const auto& [jointId, joint] : joints) {
        vec3 forces_sum = vec::fixed<3>(fill::zeros);
        for (const auto& force : joint->getForces()) {
            forces_sum += force;
        }
        auto norm = joint->getNorm();

        std::cout << "    \"" << jointId << "\" : {\n";
        std::cout << "        \"id\": " << jointId << ",\n";
        std::cout << "        \"coors\": " << vecToString(joint->getCoors()) << ",\n";
        if (norm) {
        std::cout << "        \"norm\": " << vecToString(norm.value()) <<",\n";
        }
        std::cout << "        \"is_wall\": " << (joint->getIsWall() ? "true" : "false") << ",\n";
        std::cout << "        \"is_bounded\": " << (joint->getIsBounded() ? "true" : "false") << ",\n";
        std::cout << "        \"neighbors\": " << vecToString(joint->getCanonicNeighborsIds()) <<",\n";
        std::cout << "        \"forces\": " << vecToString(forces_sum) <<",\n";
        std::cout << "        \"moments\": [\n";
        std::cout << momentsToString(joint->getMoments());
        std::cout << "        ]\n";
        std::cout << "    }" << (i < size ? "," : "") << std::endl;

        i++;
    }
    std::cout << "}}" << std::endl;
}

void printMatrixInfo(
    const mat& constraintMatrix, 
    const vec& constraintBounds,
    const VariableManager& variableManager
) {
    std::cout << std::endl << "Dimension: " << constraintMatrix.n_rows << " x " << constraintMatrix.n_cols << std::endl << std::endl;

    for (const auto& varName : variableManager.getVariableNames()) {
        std::cout << varName << ";";
    }
    std::cout << std::endl << std::endl;
    for (unsigned int row = 0; row < constraintMatrix.n_rows; row++) {
        for (unsigned int col = 0; col < constraintMatrix.n_cols; col++) {
            std::cout << std::round(100000 * constraintMatrix(row, col)) / 100000 << ";";
        }
        std::cout << " = " << constraintBounds(row) << std::endl;
    }
    std::cout << std::endl << std::endl;
    std::cout << "Bound lower:" << std::endl;
    for (const auto& lb : variableManager.getVariablesLowerBounds()) {
        std::cout << lb << ";";
    }
    std::cout << std::endl << std::endl;
    std::cout << "Bound upper:" << std::endl;
    for (const auto& ub : variableManager.getVariablesUpperBounds()) {
        std::cout << ub << ";";
    }
    std::cout << std::endl << std::endl;
}