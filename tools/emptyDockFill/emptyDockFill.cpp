#include <Configuration.h>
#include <IO.h>
#include <iostream>
#include <fstream>
#include <string>
#include <optional>

std::string createName(const Edge& edge, int alpha, int beta, int gamma) {
    std::stringstream out;
    out << "|" << alpha/90 << beta/90 << gamma/90 <<
        IO::toString(edge.side1()) <<
        IO::toString(edge.dock1()) <<
        IO::toString(edge.ori()) <<
        IO::toString(edge.dock2()) <<
        IO::toString(edge.side2()) <<
        edge.id2();
    return out.str();
}

void printConfig(const Configuration& config, const std::string& name,  const std::string& dirpath) {
    std::ofstream output;
    std::string outputPath = dirpath + name + ".in";
    output.open(outputPath.c_str());
    if(!output) {
        std::cout << "Could not open output file" << std::endl;
    }

    output << IO::toString(config);
    output.close();
}

void fillWithAngles(Configuration config, ID curr_id, int alpha, int beta, int gamma, const std::string& dirpath) {
    config.addModule(alpha, beta, gamma, curr_id);
    for (const auto& [to_id, _module] : config.getModules()) {
        if (to_id == curr_id)
            continue;
        std::optional<Edge> curr_edge = std::make_optional<Edge>(curr_id, A, XPlus, 0, XPlus, A, to_id);
        while (curr_edge.has_value()) {
            if (config.addEdge(curr_edge.value())) {
                if (config.isValid()) {
                    auto name = createName(curr_edge.value(), alpha, beta, gamma);
                    printConfig(config, name, dirpath);
                }
                config.removeEdge(curr_edge.value());
            }
            curr_edge = nextEdge(curr_edge.value());
        }
    }
}

ID getMaxId(const Configuration& config) {
    ID max = -1;
    for (const auto& [to_id, _module] : config.getModules()) {
        if (max < to_id)
            max = to_id;
    }
    return max;
}

void emptyDockFill(Configuration& config, const std::string& dirpath) {
    auto curr_id  = getMaxId(config) + 1;
    config.addModule(0, 0, 0, curr_id);
    for (int alpha : {-90, 0, 90}) {
        for (int beta : {-90, 0, 90}) {
            for (int gamma : {-90, 0, 90, 180}) {
                fillWithAngles(config, curr_id, alpha, beta, gamma, dirpath);
            }
        }
    }
}

void printHelp() {
    std::cout << "Correct use:" << std::endl;
    std::cout << "\t./emptyDockFill [pathToInput] [pathToOutputDir/]" << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cout << "Incorrect use of tool." << std::endl;
        printHelp();
        return 1;
    }

    std::ifstream input;
    input.open(argv[1]);
    if(!input) {
        std::cout << "Could not open input file" << std::endl;
        return 1;
    }

    Configuration config;
    if(!IO::readConfiguration(input, config)) {
        std::cout << "Could not read config" << std::endl;
        return 1;
    }
    input.close();
    std::string dirpath(argv[2]);
    std::string name(argv[1]);
    dirpath += name.substr(name.find_last_of("\\/") + 1, name.size());
    dirpath.erase(dirpath.size() - 3);
    emptyDockFill(config, dirpath);
    return 0;
}