#pragma once

#include <torqueComputation/linprog.hpp>
#include <configuration/rofiworld.hpp>
#include "../../src/joint.hpp"
#include "../../src/utils.hpp"
#include "../../src/variableManager.hpp"
#include <armadillo>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace rofi::torqueComputation {
    enum ComponentTypeIndex {
        Roficom,
        UmBody,
        UmShoe,
        CubeBody,
        GammaMotor,
        Motor,
        /**
         * Is used only for creating array!!
         */
        COMPONENT_TYPE_LENGTH
    };

    enum SolveTo {
        Solve,
        MatrixComposition,
        JointsCreation
    };

    /**
     * Separate configuration for each component - weights and bounds of each part.
     */
    struct ComponentConfiguration {
        std::vector<double> weights;
        std::vector<std::optional<double>> bounds;
    };

    /**
     * How to process module of desired type - check fromJSON method if new strategy is created.
     */
    class VisitModuleStrategy
    {
        public:
            VisitModuleStrategy(const ComponentConfiguration& configuration) 
                : configuration(configuration) {}

            virtual ~VisitModuleStrategy() {}

            /**
             * Create all required joints in processed module. Creates links between them and add forces.
             * @param rofiModule
             * @param roficomMap
             * @param joints
             * @param variableManager
             * @param gravitation
             */
            virtual void createJoints(
                const rofi::configuration::Module& rofiModule, 
                const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
                std::unordered_map<int, Joint*>& joints,
                VariableManager& variableManager,
                const arma::vec& gravitation
            ) const = 0;

            const ComponentConfiguration& getConfiguration() const {return configuration;}
        
        private:
            ComponentConfiguration configuration;
    };

    /**
     * Strategy for visiting universal module.
     */
    class VisitUniversalModuleStrategy : public VisitModuleStrategy
    {
    public:
        VisitUniversalModuleStrategy(const ComponentConfiguration& configuration) 
                : VisitModuleStrategy(configuration) {}

        void createJoints(
            const rofi::configuration::Module& rofiModule, 
            const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
            std::unordered_map<int, Joint*>& joints,
            VariableManager& variableManager,
            const arma::vec& gravitation
        ) const;
    };

    /**
     * Strategy for visiting pad module.
     */
    class VisitPadModuleStrategy : public VisitModuleStrategy
    {
    public:
        VisitPadModuleStrategy(const ComponentConfiguration& configuration) 
                : VisitModuleStrategy(configuration) {}

        void createJoints(
            const rofi::configuration::Module& rofiModule, 
            const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
            std::unordered_map<int, Joint*>& joints,
            VariableManager&,
            const arma::vec& gravitation
        ) const;
    };

    /**
     * Config of torque computation.
     */
    struct TorqueConfig {
        arma::vec gravitation;
        std::unordered_map<std::string, VisitModuleStrategy*> visitModuleStrategies;
        std::vector<std::tuple<int, int, arma::vec>> forces;
        std::vector<std::pair<int, int>> walls;
    };

    /**
     * Solves CLP problem based on provided rofiWorld and config. Checks if motors of modules can hold its positions.
     * @param rofiWorld
     * @param config
     * @param solveTo
     * @param printJoints
     * @param printMatrix
     * @return
     */
    OptimizeResult computeTorque(
        const rofi::configuration::RofiWorld& rofiWorld, 
        const TorqueConfig& config,
        SolveTo solveTo = SolveTo::Solve,
        bool printJoints = false,
        bool printMatrix = false
    );

    /**
     * Transforms config to JSON representation.
     * @param config
     * @return
     */
    nlohmann::json toJSON(const TorqueConfig& config);

    /**
     * Creates config from provided JSON. Allocates VisitModuleStrategies. Do not forget to release them.
     * @param json
     * @return
     */
    TorqueConfig fromJSON(const nlohmann::json& json);
}