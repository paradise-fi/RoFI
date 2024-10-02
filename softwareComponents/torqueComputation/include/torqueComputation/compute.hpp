#pragma once

#include <memory>
#include <vector>

#include <torqueComputation/linprog.hpp>
#include <configuration/rofiworld.hpp>
#include "../../src/joint.hpp"
#include "../../src/utils.hpp"
#include "../../src/variableManager.hpp"
#include <armadillo>
#include <unordered_map>

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
             * Created joints are stored in joints map.
             * @param rofiModule Module to process
             * @param roficomMap Maps roficoms from different modules to single joint ID
             * @param joints Maps joint IDs to joints
             * @param variableManager Registers newly created variable during processing of joints creation
             * @param gravitation Gravitation vector
             */
            virtual void createJoints(
                const rofi::configuration::Module& rofiModule, 
                const std::unordered_map<std::pair<int, int>, int, IntPairHash>& roficomMap,
                std::unordered_map<int, std::unique_ptr<Joint>>& joints,
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
            std::unordered_map<int, std::unique_ptr<Joint>>& joints,
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
            std::unordered_map<int, std::unique_ptr<Joint>>& joints,
            VariableManager&,
            const arma::vec& gravitation
        ) const;
    };

    /**
     * Config of torque computation.
     */
    struct TorqueConfig {
        arma::vec gravitation;
        std::unordered_map<rofi::configuration::ModuleType, std::unique_ptr<VisitModuleStrategy>> visitModuleStrategies;
        std::vector<std::tuple<int, int, arma::vec>> forces;
        std::vector<std::pair<int, int>> walls;
    };

    /**
     * Solves CLP problem based on provided rofiWorld and config. Checks if motors of modules can hold its positions.
     * @param rofiWorld RoFI world to process.
     * @param config Configuration of physical properties of modules. Also contain strategy how each type of module should be processed.
     * @param solveTo Used to set stage up to algorithm is run. Either up to transform world to joints, compose system of euqations or whole problem is solved.
     * @param printJoints Print system of joints as JSON, can be used as input for system drawing script.
     * @param printMatrix Print composed matrix as CSV to see what was done.
     * @return Result of CLP computation
     */
    OptimizeResult computeTorque(
        const rofi::configuration::RofiWorld& rofiWorld, 
        const TorqueConfig& config,
        SolveTo solveTo = SolveTo::Solve,
        bool printJoints = false,
        bool printMatrix = false
    );
}
