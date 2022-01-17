//
// Created by maki on 9.3.19.
//

#ifndef ROFI_GENERATOR_H
#define ROFI_GENERATOR_H

#include <legacy/configuration/Configuration.h>
#include <cmath>
#include <set>

class Generator{
public:
    /**
     * This function generates configurations between initConf and goalConf.
     * The result saves to vector vec.
     * The size of the steps (the difference between two consecutive configurations)
     * depends on the parameters given.
     *
     * @param initConf initial configuration
     * @param goalConf goal configuration
     * @param vec vector of generated configurations
     * @param maxPhi maximal angle difference in one step
     * @param reconnectionPics number of steps for reconnection
     */
    void generate(const Configuration& initConf, const Configuration& goalConf, std::vector<Configuration>& vec,
                  double maxPhi, unsigned int reconnectionPics){
        maxPhi = std::abs(maxPhi);
        unsigned int moduleSteps = getStepsCount(initConf, goalConf, maxPhi);
        unsigned int totalSteps = moduleSteps;
        if (initConf.getEdges() != goalConf.getEdges()){
            totalSteps = moduleSteps > reconnectionPics ? moduleSteps : reconnectionPics;
        }
        std::vector<Edge> onlyInitEdges{}, onlyGoalEdges{}, sameEdges{};
        sortEdges(initConf, goalConf, onlyInitEdges, onlyGoalEdges, sameEdges);
        for (unsigned int i = 1; i < totalSteps; i++){
            Configuration c;
            generateOneStep(initConf, goalConf, c, moduleSteps, maxPhi, reconnectionPics, i,
                            onlyInitEdges, onlyGoalEdges, sameEdges);
            vec.push_back(c);
        }
    }

    /**
     * This function returns count of configurations to be generated between initConf and goalConf
     * to have maximal angle difference maxPhi between two consecutive configurations.
     *
     * @param initConf initial configuration
     * @param goalConf goal configuration
     * @param maxPhi maximal angle difference between two consecutive configurations
     * @return count of configurations to be generated
     */
    unsigned int getStepsCount(const Configuration& initConf, const Configuration& goalConf, double maxPhi){
        double maxAngleMove = getMaxAngleMove(initConf, goalConf);
        return static_cast<unsigned int>(std::ceil(maxAngleMove / maxPhi));
    }


private:
    /**
     * This function sorts edges of configuration into three categories.
     *
     * @param initConf initial configuration
     * @param goalConf goal configuration
     * @param onlyInitEdges edges to be removed during generated configs - are in initConf, but not in goalConf
     * @param onlyGoalEdges edges to be added during generated configs - are not in initConf, but in goalConf
     * @param sameEdges edges which do not change - are in both initConf and goalConf
     */
    void sortEdges(const Configuration& initConf, const Configuration& goalConf, std::vector<Edge>& onlyInitEdges,
            std::vector<Edge>& onlyGoalEdges, std::vector<Edge>& sameEdges){
        std::set<ID> ids = getIDs(initConf);
        std::vector<Edge> initEdges{};
        std::vector<Edge> goalEdges{};
        for (unsigned int id : ids){
            for (const auto& e :  initConf.getEdges(id)){
                initEdges.push_back(e);
            }
            for (const auto& e : goalConf.getEdges(id)){
                goalEdges.push_back(e);
            }
        }
        for (const auto& edge : initEdges){
            if (goalConf.findEdge(edge)){
                sameEdges.push_back(edge);
            } else {
                onlyInitEdges.push_back(edge);
            }
        }
        for (const auto& edge : goalEdges){
            if (!initConf.findEdge(edge)){
                onlyGoalEdges.push_back(edge);
            }
        }
    }

    /**
     * This function generates modules for currently generated configuration.
     *
     * @param initConf initial configuration
     * @param goalConf goal configuration
     * @param currConf currently generated configuration
     * @param moduleSteps how many steps need to be generated to get angles of all modules to the goal position
     * @param maxPhi maximal angle difference between two consecutive configurations
     * @param currentStep step number which is now generated
     */
    void generateOneModuleStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
            unsigned int moduleSteps, double maxPhi, unsigned int currentStep){
        for (const auto& [id1, mod1] : initConf.getModules()){
            const auto& mod2 = goalConf.getModules().at(id1);
            if (mod1 == mod2){
                currConf.addModule(mod1.getJoint(Alpha), mod1.getJoint(Beta), mod1.getJoint(Gamma), mod1.getId());
                continue;
            }
            if (currentStep >= moduleSteps){
                currConf.addModule(mod2.getJoint(Alpha), mod2.getJoint(Beta), mod2.getJoint(Gamma), mod2.getId());
                continue;
            }
            double alpha = countStep(mod1.getJoint(Alpha), mod2.getJoint(Alpha), maxPhi, currentStep);
            double beta = countStep(mod1.getJoint(Beta), mod2.getJoint(Beta), maxPhi, currentStep);
            double gamma = countStep(mod1.getJoint(Gamma), mod2.getJoint(Gamma), maxPhi, currentStep);
            currConf.addModule(alpha, beta, gamma, mod1.getId());
        }
    }

    /**
     * This function generates edges for currently generated configuration.
     *
     * @param currConf currently generated configuration
     * @param reconnectionPics number of steps for reconnection
     * @param currentStep step number which is now generated
     * @param onlyInitEdges edges to be removed during generated configs - are in initConf, but not in goalConf
     * @param onlyGoalEdges edges to be added during generated configs - are not in initConf, but in goalConf
     * @param sameEdges edges which do not change - are in both initConf and goalConf
     */
    void generateOneEdgeStep(Configuration& currConf, unsigned int reconnectionPics, unsigned int currentStep,
                             const std::vector<Edge>& onlyInitEdges, const std::vector<Edge>& onlyGoalEdges,
                             const std::vector<Edge>& sameEdges){
        for (const auto& e : sameEdges){
            currConf.addEdge(e);
        }
        double coeff = currentStep / static_cast<double>(reconnectionPics);
        if (coeff > 1){ coeff = 1;}
        for (auto e : onlyInitEdges){
            e.setOnCoeff(1 - coeff);
            currConf.addEdge(e);
        }
        for (auto e : onlyGoalEdges){
            e.setOnCoeff(coeff);
            currConf.addEdge(e);
        }
    }

    /**
     * This function generates one configuration.
     *
     * @param initConf initial configuration
     * @param goalConf goal configuration
     * @param currConf currently generated configuration
     * @param moduleSteps how many steps need to be generated to get angles of all modules to the goal position
     * @param maxPhi maximal angle difference between two consecutive configurations
     * @param reconnectionPics number of steps for reconnection
     * @param currentStep step number which is now generated
     * @param onlyInitEdges edges to be removed during generated configs - are in initConf, but not in goalConf
     * @param onlyGoalEdges edges to be added during generated configs - are not in initConf, but in goalConf
     * @param sameEdges edges which do not change - are in both initConf and goalConf
     */
    void generateOneStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
            unsigned int moduleSteps, double maxPhi, unsigned int reconnectionPics,
            unsigned int currentStep, const std::vector<Edge>& onlyInitEdges, const std::vector<Edge>& onlyGoalEdges,
            const std::vector<Edge>& sameEdges){
        generateOneModuleStep(initConf, goalConf, currConf, moduleSteps, maxPhi, currentStep);
        generateOneEdgeStep(currConf, reconnectionPics, currentStep,
                onlyInitEdges, onlyGoalEdges, sameEdges);

    }

    /**
     * This function counts angle of generated module.
     *
     * @param a value in initConf
     * @param b value in goalConf
     * @param maxPhi maximal angle difference between two consecutive configurations
     * @param step step number which is now generated
     * @return value of angle in generated configuration
     */
    double countStep(double a, double b, double maxPhi, unsigned int step){
        if (a < b){
            if (std::abs(b - a) > 180){
                //gamma angle must go through -180=180
                if (a - step * maxPhi > -180){
                    return a - step * maxPhi;
                }
                if (a - step * maxPhi + 360 >= b) {
                    return a - step * maxPhi + 360;
                }
                return b;
            }
            if (a + step * maxPhi <= b){
                return a + step * maxPhi;
            }
            return b;
        }
        // a >= b
        if (std::abs(a - b) > 180){
            //gamma angle must go through -180=180
            if (a + step * maxPhi <= 180){
                return a + step * maxPhi;
            }
            if (a + step * maxPhi - 360 <= b){
                return a + step * maxPhi - 360;
            }
            return b;
        }
        if (a - step * maxPhi >= b){
            return a - step * maxPhi;
        }
        return b;

    }

    /**
     * This function returns maximal difference of angle in configuration c1 and c2.
     *
     * @param c1 initial configuration
     * @param c2 goal configuration
     * @return maximal angle difference
     */
    double getMaxAngleMove(const Configuration& c1, const Configuration& c2){
        double maxAngleMove = 0;
        for ( const auto& [id1, mod1] : c1.getModules() ){
            const auto& mod2 = c2.getModules().at(id1);
            double currentMove = countDiffAngle(mod1, mod2);
            if (currentMove > maxAngleMove){
                maxAngleMove = currentMove;
            }
        }
        return maxAngleMove;
    }

    /**
     * This function returns maximal difference of angle in one module.
     *
     * @param m1 module in initial configuration
     * @param m2 module in goal configuration
     * @return maximal difference of angle in one module.
     */
    double countDiffAngle(const Module& m1, const Module& m2){
        double alpha = std::abs(m1.getJoint(Alpha) - m2.getJoint(Alpha));
        double beta = std::abs(m1.getJoint(Beta) - m2.getJoint(Beta));
        double gamma = std::abs(m1.getJoint(Gamma) - m2.getJoint(Gamma));
        if (gamma > 180){
            gamma = 360 - gamma;
        }
        return std::max(std::max(alpha, beta), gamma);
    }

    /**
     * This function returns IDs of modules in configuration.
     *
     * @param config configuration
     * @return IDs of modules in configuration
     */
    std::set<ID> getIDs(const Configuration& config){
        std::set<ID> res;
        for (const auto& [id, _] : config.getModules()){
            res.insert(id);
        }
        return res;
    }
};

#endif //ROFI_GENERATOR_H
