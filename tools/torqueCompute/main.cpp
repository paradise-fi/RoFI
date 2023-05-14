#include <torqueComputation/compute.hpp>
#include <torqueComputation/serialization.hpp>
#include <configuration/serialization.hpp>
#include <configuration/universalModule.hpp>
#include <parsing/parsing_lite.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <cmath>
#include <chrono>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <getopt.h>
#include <filesystem>


using json = nlohmann::json;
using namespace rofi::configuration;
using namespace rofi::torqueComputation;
using namespace arma;

enum ArgsEnum {
    Empty = 0,
    Help = 1,
    SerializeWorld = 2,
    SerializeConfig = 4,
    VerboseJoints = 8,
    VerboseMatrix = 16
};

inline bool endsWith(const std::string& value, const std::string& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

ArgsEnum parseOpts(int argc, char* argv[], std::unordered_map<std::string, int>& argsMap) {
    static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"verbose", optional_argument, 0, 'v'},
            {"serialize", required_argument, 0, 's'},
            {"solveTo", required_argument, 0, 't'},
            {"repeat", required_argument, 0, 'r'},
            {0, 0, 0, 0}
    };
    ArgsEnum args = ArgsEnum::Empty;
    int index = 0;
    int opt;
    argsMap.emplace("solve", SolveTo::Solve);
    argsMap.emplace("repeat", 10);

    std::string stringOption;
    while ((opt = getopt_long(argc, argv, "hv::s:t:r:", long_options, &index)) != -1) {
        switch (opt) {
            case 's':
                stringOption = std::string(optarg);
                if (stringOption == "world") {
                    args = static_cast<ArgsEnum>(args | ArgsEnum::SerializeWorld);
                }
                else if (stringOption == "config") {
                    args = static_cast<ArgsEnum>(args | ArgsEnum::SerializeConfig);
                }
                else {
                    throw std::invalid_argument("Invalid option for --serialize\n");
                }
                break;
            case 'v':
                if ((optarg == NULL) || std::stoi(optarg) == 0) {
                    args = static_cast<ArgsEnum>(args | ArgsEnum::VerboseJoints);
                }
                else {
                    args = static_cast<ArgsEnum>(args | ArgsEnum::VerboseMatrix);
                }
                break;
            case 't':
                stringOption = std::string(optarg);
                if (stringOption == "matrix") {
                    argsMap["solve"] = SolveTo::MatrixComposition;
                }
                else if (stringOption == "joints") {
                    argsMap["solve"] = SolveTo::JointsCreation;
                }
                else if (stringOption == "solve") {
                    argsMap["solve"] = SolveTo::Solve;
                }
                else {
                    throw std::invalid_argument("Invalid option for --solveTo\n");
                }
                break;
            case 'r':
                argsMap["repeat"] = std::stoi(optarg);
                break;
            case 'h':
                args = static_cast<ArgsEnum>(args | ArgsEnum::Help);
                break;
        }
    }
    return args;
}

void printHelp() {
    std::cout << "Usage: <program_name> <pathToWorld> <pathToConfig> [OPTIONS]" << std::endl
              << "    -s <option>, --serialize=<option>   Choose serialization option: world, config" << std::endl
              << "    -v[option]                          Verbose output. Display all joints properties. Option 1 to print matrix." << std::endl
              << "    -t <option>, --solveTo=<option>     Choose solve option: solve (complete), joints (up to joints creation)," << std::endl
              << "                                        matrix (up to matrix creation) -- default solve" << std::endl
              << "    -r <option>, --repeat=<option>      How many times should be measurement performed -- default 10." << std::endl
              << "    -h, --help                          Print this help message" << std::endl;
}

RofiWorld createWorld(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::logic_error("World file does not exist!");
    }
    /*if (!endsWith(path, "json") && !endsWith(path, "rofi")) {
        throw std::logic_error("Invalid format");
    }*/

    RofiWorld world;
    std::ifstream file(path);

    if (endsWith(path, "json")) {
        json data = json::parse(file);
        world = rofi::configuration::serialization::fromJSON(data);
    } else {
        world = readOldConfigurationFormat(file);
        rofi::parsing::fixateRofiWorld(world);
    }
    file.close();
    return world;
}

TorqueConfig createConfig(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::logic_error("Config file does not exist!");
    }
    if (!endsWith(path, "json")) {
        throw std::logic_error("Invalid format");
    }

    std::ifstream file(path);
    TorqueConfig config;
    try {
        json data = json::parse(file);
        config = fromJSON(data);
    }
    catch (std::exception& e) {
        std::cerr << "Invalid TORQUE Config!!" << std::endl;
    }

    file.close();
    return config;
}


int main( int argc, char * argv[] )
{
    if (argc < 3) {
        printHelp();
        return 0;
    }
    auto worldPath = std::string(argv[1]);
    auto configPath = std::string(argv[2]);

    std::unordered_map<std::string, int> argsMap;
    ArgsEnum args = ArgsEnum::Empty;
    try {
        args = parseOpts(argc, argv, argsMap);
    }
    catch (std::invalid_argument& e) {
        cerr << e.what();
        printHelp();
        return 1;
    }
    if (args != ArgsEnum::Empty) {
        argsMap["repeat"] = 1;
    }
    SolveTo solveTo = static_cast<SolveTo>(argsMap.at("solve"));
    int repeat = argsMap.at("repeat");

    if (args & ArgsEnum::Help) {
        printHelp();
        return 0;
    }

    auto world = createWorld(worldPath);
    auto config = createConfig(configPath);

    if (args & ArgsEnum::SerializeWorld) {
        std::cout << rofi::configuration::serialization::toJSON(world) << std::endl;
        return 0;
    }

    if (args & ArgsEnum::SerializeConfig) {
        std::cout << toJSON(config) << std::endl;
        return 0;
    }

    long double elapsed = INFINITY;
    bool result = false;

    for (int i = 0; i < repeat; i++) {
        auto start = std::chrono::steady_clock::now();
        result = computeTorque(
                world,
                config,
                solveTo,
                args & ArgsEnum::VerboseJoints,
                args & ArgsEnum::VerboseMatrix
        ).feasible;
        auto end = std::chrono::steady_clock::now();

        long double current = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        if (current < elapsed) {
            elapsed = current;
        }
    }

    if (args == ArgsEnum::Empty) {
        std::cout << "Elapsed time in nanoseconds: "
                  << elapsed
                  << " ns" << std::endl;

        std::cout << "Elapsed time in microseconds: "
                  << (elapsed / 1000.0)
                  << " µs" << std::endl;

        std::cout << "Elapsed time in milliseconds: "
                  << (elapsed / 1000000.0)
                  << " ms" << std::endl;

        std::cout << endl << "RESULT: " << (result ? "SUCCESS" : "fail") << std::endl << std::endl;
    }
    return 0;
}