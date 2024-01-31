#include <torqueComputation/serialization.hpp>

using namespace nlohmann;
using namespace arma;
using namespace rofi::torqueComputation;
using namespace rofi::configuration;

namespace rofi::torqueComputation {
    std::string toLowercase(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                        [](unsigned char c){ return std::tolower(c); });
        return result;
    }

    std::string moduleTypeToString(ModuleType moduleType) {
        switch (moduleType) {
            case ModuleType::Unknown:
                return "Unknown";
            case ModuleType::Universal:
                return "Universal";
            case ModuleType::Pad:
                return "Pad";
            case ModuleType::Cube:
                return "Cube";
            case ModuleType::Support:
                return "Support";
            default:
                throw std::out_of_range("Invalid module type");
        }
    }

    ModuleType moduleTypeFromString(const std::string& str) {
        const std::string lower = toLowercase(str);
        if (lower == "unknown") {
            return ModuleType::Unknown;
        } else if (lower == "universal") {
            return ModuleType::Universal;
        } else if (lower == "pad") {
            return ModuleType::Pad;
        } else if (lower == "cube") {
            return ModuleType::Cube;
        } else if (lower == "support") {
            return ModuleType::Support;
        } else {
            throw std::invalid_argument("Invalid module type string: " + str);
        }
    }

    std::string componentTypeToString(ComponentTypeIndex componentType) {
        switch (componentType) {
            case ComponentTypeIndex::Roficom:
                return "Roficom";
            case ComponentTypeIndex::UmBody:
                return "UmBody";
            case ComponentTypeIndex::UmShoe:
                return "UmShoe";
            case ComponentTypeIndex::CubeBody:
                return "CubeBody";
            case ComponentTypeIndex::Cylinder:
                return "Cylinder";
            case ComponentTypeIndex::GammaMotor:
                return "GammaMotor";
            case ComponentTypeIndex::Motor:
                return "Motor";
            default:
                throw std::invalid_argument("Invalid component type");
        }
    }

    ComponentTypeIndex componentTypeFromString(const std::string& str) {
        const std::string lower = toLowercase(str);
        if (lower == "roficom") {
            return ComponentTypeIndex::Roficom;
        } else if (lower == "umbody") {
            return ComponentTypeIndex::UmBody;
        } else if (lower == "umshoe") {
            return ComponentTypeIndex::UmShoe;
        } else if (lower == "cubebody") {
            return ComponentTypeIndex::CubeBody;
        } else if (lower == "cylinder") {
            return ComponentTypeIndex::Cylinder;
        } else if (lower == "gammamotor") {
            return ComponentTypeIndex::GammaMotor;
        } else if (lower == "motor") {
            return ComponentTypeIndex::Motor;
        } else {
            throw std::out_of_range("Invalid component type string: " + str);
        }
    }

    nlohmann::json visitModuleStrategyToJSON(const VisitModuleStrategy& strategy) {
        const auto& config = strategy.getConfiguration();
        json res = json::object();
        for (size_t i = 0; i < config.weights.size(); i++) {
            auto key = componentTypeToString(static_cast<ComponentTypeIndex>(i));
            if (0 < config.weights[i]) {
                res["weights"][key] = config.weights[i];
            }
            if (config.bounds[i].has_value()) {
                res["bounds"][key] = config.bounds[i].value();
            }
        }
        return res;
    }

    VisitModuleStrategy* jsonToVisitModuleStrategy(ModuleType moduleType, const json& j)
    {
        ComponentConfiguration config;

        config.weights.resize(ComponentTypeIndex::COMPONENT_TYPE_LENGTH, 0);
        if (j.contains("weights")) {
            auto jWeights = j["weights"];
            for (auto it = jWeights.begin(); it != jWeights.end(); ++it) {
                int i = static_cast<int>(componentTypeFromString(it.key()));
                double weight = it.value().get<double>();
                config.weights[i] = weight;
            }
        }

        config.bounds.resize(ComponentTypeIndex::COMPONENT_TYPE_LENGTH);
        if (j.contains("bounds")) {
            auto jBounds = j["bounds"];
            for (auto it = jBounds.begin(); it != jBounds.end(); ++it) {
                int i = static_cast<int>(componentTypeFromString(it.key()));
                double bound = it.value().get<double>();
                config.bounds[i] = bound;
            }
        }
        
        switch(moduleType) {
            case ModuleType::Universal:
                return new VisitUniversalModuleStrategy(config);
            case ModuleType::Pad:
                return new VisitPadModuleStrategy(config);
            default:
                std::string msg = "TorqueConfig error. VisitModuleStrategy for module type {" + 
                moduleTypeToString(moduleType) + "} must be programmed and added to parser.";
                throw std::logic_error(msg);
        }
    }

    nlohmann::json toJSON(const TorqueConfig& config) {
        json res;
        res["gravitation"] = config.gravitation;

        for (const auto& [moduleType, strategy] : config.visitModuleStrategies) {
            res["visitModuleStrategies"][moduleTypeToString(moduleType)] = visitModuleStrategyToJSON(*strategy);
        }

        for (const auto& [moduleId, componentId, force] : config.forces) {
            json jForce;
            jForce["moduleId"] = moduleId;
            jForce["componentId"] = componentId;
            jForce["force"] = force;
            res["forces"].push_back(jForce);
        }

        for (const auto& [moduleId, componentId] : config.walls) {
            json jWall;
            jWall["moduleId"] = moduleId;
            jWall["componentId"] = componentId;
            res["walls"].push_back(jWall);
        }

        return res;
    }

    TorqueConfig fromJSON(const nlohmann::json& j) {
        TorqueConfig config;
        if (!j.contains("gravitation") ) {
            throw std::logic_error( "Cannot find gravitation in config json" );
        }
        if (!j.contains("visitModuleStrategies") ) {
            throw std::logic_error( "Cannot find visitModuleStrategies in config json" );
        }

        config.gravitation = vec(j.at("gravitation").get<std::vector<double>>());

        for (auto it = j["visitModuleStrategies"].begin(); it != j["visitModuleStrategies"].end(); ++it) {
            auto moduleType = moduleTypeFromString(it.key());
            auto* strategy = jsonToVisitModuleStrategy(moduleType, it.value());
            config.visitModuleStrategies.emplace(moduleType, std::unique_ptr<VisitModuleStrategy>(strategy));
        }

        if (j.contains("forces")) {
            for (const auto& jForce : j["forces"]) {
                int moduleId = jForce.at("moduleId").get<int>();
                int componentId = jForce.at("componentId").get<int>();
                arma::vec force = vec(jForce.at("force").get<std::vector<double>>());
                config.forces.emplace_back(moduleId, componentId, force);
            }
        }

        if (j.contains("walls")) {
            for (const auto& jWall : j["walls"]) {
                int moduleId = jWall.at("moduleId").get<int>();
                int componentId = jWall.at("componentId").get<int>();
                config.walls.emplace_back(moduleId, componentId);
            }
        }
        return config;
    }
}
