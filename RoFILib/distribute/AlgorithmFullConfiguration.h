//
// Created by xmichel on 5/20/19.
//

#ifndef ROFI_ALGORITHMFULLCONFIGURATION_H
#define ROFI_ALGORITHMFULLCONFIGURATION_H


#include "../reconfig/Algorithms.h"
#include "DistributedModule.h"

class AlgorithmFullConfiguration : public DistributedModule {
public:
    AlgorithmFullConfiguration(unsigned int id, std::ifstream &inStream, std::ifstream &trgStream);

    std::string toStringCurrConfiguration(int step) const;
    std::string toStringTrgConfiguration(int step) const;

    void reconfigurate(EvalFunction *eval);

private:
    Configuration currConfiguration;
    Configuration trgConfiguration;

    void shareCurrConfigurations() { shareConfigurations(currModule, currConfiguration); }
    void shareTrgConfigurations() { shareConfigurations(trgModule, trgConfiguration); }

    void shareConfigurations(const DistributedModuleProperties &module, Configuration &configuration);

    EdgeList createEdgeListFromEdges(const std::vector<Edge> &edges, unsigned int id) const;

    void executeDiff(const Action &action, int step);

    std::vector<Action> createDiffs(const std::vector<Configuration> &path) const;

    void optimizePath(std::vector<Action> &path) const;

    bool joinActions(std::vector<Action> &path) const;

    void swapActions(std::vector<Action> &path) const;

};


#endif //ROFI_ALGORITHMFULLCONFIGURATION_H
