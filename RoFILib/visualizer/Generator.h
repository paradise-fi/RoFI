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
            for( const auto& edge : edges1){
                CurrConf.addEdge(edge.getId1(), edge.getSide1(), edge.getDock1(), edge.getOri(), edge.getDock2(),
                        edge.getSide2(), edge.getId2());
            }
        }
    }

private:
    double countStep(double a, double b, unsigned int totalSteps, unsigned int step){
        return a + (((b - a) * step) / totalSteps);
    }
};

#endif //ROFI_GENERATOR_H
