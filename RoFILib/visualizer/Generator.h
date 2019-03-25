//
// Created by maki on 9.3.19.
//

#ifndef ROFI_GENERATOR_H
#define ROFI_GENERATOR_H

#include "../Configuration.h"
#include <cmath>

class Generator{
public:
    void generate(const Configuration& InitConf, const Configuration& GoalConf, std::vector<Configuration>& vec,
            unsigned int steps){
        //TODO promyslet steps +-1
        //steps = 5 -> 4 meziobrazky
        for (unsigned int i = 1; i < steps; i++){
            Configuration c;
            generateOneStep(InitConf, GoalConf, c, steps, i);
            vec.push_back(c);
        }
    }

    void generateOneStep(const Configuration& InitConf, const Configuration& GoalConf,
            Configuration& CurrConf, unsigned int totalSteps, unsigned int currentStep){
        for ( const auto& [id1, mod1] : InitConf.getModules() ){
            const auto& mod2 = GoalConf.getModules().at(id1);
            if (mod1 == mod2){
                CurrConf.addModule(mod1.getJoint(Alpha), mod1.getJoint(Beta), mod1.getJoint(Gamma), mod1.getId());
                continue;
            }
            double alpha = countStep(mod1.getJoint(Alpha), mod2.getJoint(Alpha), totalSteps, currentStep);
            double beta = countStep(mod1.getJoint(Beta), mod2.getJoint(Beta), totalSteps, currentStep);
            double gamma = countStep(mod1.getJoint(Gamma), mod2.getJoint(Gamma), totalSteps, currentStep);
            CurrConf.addModule(alpha, beta, gamma, mod1.getId());
        }
        for ( const auto& [id1, edges1] : InitConf.getEdges()){
            for (const std::optional<Edge>& edgeOpt : edges1){
                if (!edgeOpt.has_value())
                    continue;
                const Edge& edge = edgeOpt.value();
                CurrConf.addEdge({edge.id1, edge.side1, edge.dock1, edge.ori, edge.dock2,
                                  edge.side2, edge.id2});
            }
        }
    }

    void generateEdgeChange(const Configuration& initConf, const Configuration& goalConf,
            std::vector<Configuration>& vec, unsigned int reconnectionPics){
        if (sameEdges(initConf, goalConf)){
            return;
        }
        unsigned int count = reconnectionPics / 2;
        Configuration c1;
        Configuration c2 = goalConf;
        if (vec.empty()){ //no inter config
            c1 = initConf;
        } else {
            c1 = vec.at(vec.size() - 1);
        }
        for (unsigned int i = 0; i < count; i++){
            vec.push_back(c1);
        }
        for (unsigned int j = 0; j < count; j++){
            vec.push_back(c2);
        }
    }

    /**
     * generates inter steps betweeen initConf and goalConf
     * @param initConf
     * @param goalConf
     * @param vec saves result to this vector
     * @param maxPhi max angle change per configuration generated (per picture)
     * @param reconnectionPics count of configs (pictures) to display reconnection of modules
     */
    void generate(const Configuration& initConf, const Configuration& goalConf, std::vector<Configuration>& vec,
            double maxPhi, unsigned int reconnectionPics){
        unsigned  int steps = getStepsCount(initConf, goalConf, maxPhi);
        generate(initConf, goalConf, vec, steps);
        generateEdgeChange(initConf, goalConf, vec, reconnectionPics);
    }

/*
    void generate(const Configuration& initConf, const Configuration& goalConf, std::vector<Configuration>& vec,
            double maxPhi){
        maxPhi = std::abs(maxPhi);
        double maxAngleMove = getMaxAngleMove(initConf, goalConf);
        auto steps = static_cast<unsigned int>(std::ceil(maxAngleMove / maxPhi));
        for (unsigned int i = 0; i < steps; i++){
            Configuration c;
            generateOneStep(initConf, goalConf, c, i, maxPhi);
        }

    }


    void generateOneStep(const Configuration& initConf, const Configuration& goalConf, Configuration& currConf,
           unsigned int currentStep, double maxPhi){
        for ( const auto& [id1, mod1] : initConf.getModules() ){
            const auto& mod2 = goalConf.getModules().at(id1);
            if (mod1 == mod2){
                currConf.addModule(mod1.getJoint(Alpha), mod1.getJoint(Beta), mod1.getJoint(Gamma), mod1.getId());
                continue;
            }
            double alpha = countStep(mod1.getJoint(Alpha), mod2.getJoint(Alpha), currentStep, maxPhi);
            double beta = countStep(mod1.getJoint(Beta), mod2.getJoint(Beta), currentStep, maxPhi);
            double gamma = countStep(mod1.getJoint(Gamma), mod2.getJoint(Gamma), currentStep, maxPhi);
            currConf.addModule(alpha, beta, gamma, id1);
        }
        for ( const auto& [id1, edges1] : initConf.getEdges()){
            for( const auto& edge : edges1){
                currConf.addEdge(edge.getId1(), edge.getSide1(), edge.getDock1(), edge.getOri(), edge.getDock2(),
                                 edge.getSide2(), edge.getId2());
            }
        }
    }
*/

    unsigned int getStepsCount(const Configuration& initConf, const Configuration& goalConf, double maxPhi){
        maxPhi = std::abs(maxPhi);
        double maxAngleMove = getMaxAngleMove(initConf, goalConf);
        return static_cast<unsigned int>(std::ceil(maxAngleMove / maxPhi));
    }

private:
    double countStep(double a, double b, unsigned int totalSteps, unsigned int step){
        return a + (((b - a) * step) / totalSteps);
    }
/*
    double countStep(double a, double b, unsigned int step, double maxPhi){
        if (a < b){
            if (a + step * maxPhi <= b){
                return a + step * maxPhi;
            }
            return b;
        } // a >= b
        if (a  - step * maxPhi >= b){
            return a - step * maxPhi;
        }
        return b;
    }
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

    double countDiffAngle(const Module& m1, const Module& m2){
        double alpha = std::abs(m1.getJoint(Alpha) - m2.getJoint(Alpha));
        double beta = std::abs(m1.getJoint(Beta) - m2.getJoint(Beta));
        double gamma = std::abs(m1.getJoint(Gamma) - m2.getJoint(Gamma));
        return std::max(std::max(alpha, beta), gamma);
    }

    bool sameEdges(const Configuration& c1, const Configuration& c2){
        return c1.getEdges() == c2.getEdges();
    }
};

#endif //ROFI_GENERATOR_H
