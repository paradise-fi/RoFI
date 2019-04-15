//
// Created by maki on 9.3.19.
//

#ifndef ROFI_GENERATOR_H
#define ROFI_GENERATOR_H

#include "../Configuration.h"
#include <cmath>

class Generator{
public:
    /**
 * generates inter steps between initConf and goalConf
 * @param initConf
 * @param goalConf
 * @param vec saves result to this vector
 * @param maxPhi max angle change per configuration generated (per picture)
 * @param reconnectionPics count of configs (pictures) to display reconnection of modules
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
            generateOneStep(initConf, goalConf, c, moduleSteps, maxPhi, reconnectionPics, totalSteps, i,
                            onlyInitEdges, onlyGoalEdges, sameEdges);
            vec.push_back(c);
        }
    }

    /**
     * returns count of configurations need to generate to have maximal angle difference
     * @param initConf
     * @param goalConf
     * @param maxPhi maximal difference angle between two configs
     * @return
     */
    unsigned int getStepsCount(const Configuration& initConf, const Configuration& goalConf, double maxPhi){
        double maxAngleMove = getMaxAngleMove(initConf, goalConf);
        return static_cast<unsigned int>(std::ceil(maxAngleMove / maxPhi));
    }


private:
    void sortEdges(const Configuration& initConf, const Configuration& goalConf, std::vector<Edge>& onlyInitEdges,
            std::vector<Edge>& onlyGoalEdges, std::vector<Edge>& sameEdges){
        ID highestId = getHighestId(initConf);
        std::vector<Edge> initEdges{};
        std::vector<Edge> goalEdges{};
        for (unsigned int id = 0; id <= highestId; id++){
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

    void generateOneModuleStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
            unsigned int moduleSteps, double maxPhi, unsigned int totalSteps, unsigned int currentStep){
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

    void generateOneEdgeStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
                             unsigned int reconnectionPics, unsigned int totalSteps, unsigned int currentStep,
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

    void generateOneStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
            unsigned int moduleSteps, double maxPhi, unsigned int reconnectionPics, unsigned int totalSteps,
            unsigned int currentStep, const std::vector<Edge>& onlyInitEdges, const std::vector<Edge>& onlyGoalEdges,
            const std::vector<Edge>& sameEdges){
        generateOneModuleStep(initConf, goalConf, currConf, moduleSteps, maxPhi, totalSteps, currentStep);
        generateOneEdgeStep(initConf, goalConf, currConf, reconnectionPics, totalSteps, currentStep,
                onlyInitEdges, onlyGoalEdges, sameEdges);

    }

    double countStep(double a, double b, double maxPhi, unsigned int step){
        if (a < b){
            if (a + step * maxPhi <= b){
                return a + step * maxPhi;
            }
            return b;
        }
        // a >= b
        if (a - step * maxPhi >= b){
            return a - step * maxPhi;
        }
        return b;

    }

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

    double countDiffAngle(const Module& m1, const Module& m2){
        double alpha = std::abs(m1.getJoint(Alpha) - m2.getJoint(Alpha));
        double beta = std::abs(m1.getJoint(Beta) - m2.getJoint(Beta));
        double gamma = std::abs(m1.getJoint(Gamma) - m2.getJoint(Gamma));
        return std::max(std::max(alpha, beta), gamma);
    }

    bool sameEdges(const Configuration& c1, const Configuration& c2){
        return c1.getEdges() == c2.getEdges();
    }

    ID getHighestId(const Configuration& config){
        ID highest = config.getModules().at(0).getId();
        for (const auto& [id, _] : config.getModules()){
            if (id > highest){
                highest = id;
            }
        }
        return highest;
    }
};

#endif //ROFI_GENERATOR_H
