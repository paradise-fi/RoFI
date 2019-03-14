//
// Created by maki on 13.3.19.
//

#ifndef ROFI_ANIMATOR_H
#define ROFI_ANIMATOR_H

#include "../Configuration.h"
#include "Generator.h"
#include "Visualizer.h"
#include <vector>

class Animator{
public:
    void visualizeMainConfigs(std::vector<Configuration>& mainConfigs, double maxPhi, unsigned int reconnectionPics,
            const std::string& path, bool savePicture, const Camera& cameraStart, const Camera& cameraEnd){
        std::vector<Configuration> allConfigs;
        bool firstConfig = true;
        if (mainConfigs.empty()){
            return;
        }
        if (mainConfigs.size() == 1){
            allConfigs.push_back(mainConfigs.at(0));
        } else {
        for (unsigned long i = 0; i < mainConfigs.size() - 2; i++){
            Configuration& c1 = mainConfigs.at(i);
            Configuration& c2 = mainConfigs.at(i + 1);
            std::vector<Configuration> generatedConfigs;
            Generator generator;
            generator.generate(c1, c2, generatedConfigs, maxPhi, reconnectionPics);
            if (firstConfig){
                allConfigs.push_back(c1);
                firstConfig = false;
            }
            for (Configuration& c : generatedConfigs){
                allConfigs.push_back(c);
            }
            allConfigs.push_back(c2);

        }
        }
        visualizeAllConfigs(allConfigs, path, savePicture, cameraStart, cameraEnd);
    }

    void visualizeAllConfigs(std::vector<Configuration>& allConfigs, const std::string& path, bool savePicture,
            Camera cameraStart, Camera cameraEnd){
        unsigned long step = 0;
        cameraStart.setCameraMassCenter(allConfigs.at(0).massCenter());
        cameraEnd.setCameraMassCenter(allConfigs.at(0).massCenter());
        for (Configuration& c : allConfigs){
            Camera camera = countCameraMove(cameraStart, cameraEnd, step, allConfigs.size() - 1);
            visualizeOneConfig(c, path, savePicture, step, camera);
            step++;
        }
    }

    void visualizeOneConfig(Configuration& config, const std::string& path, bool savePicture,
            unsigned long step, const Camera& camera){
        Visualizer visualizer;
        config.computeRotations();
        std::string filename = getFilename(path, step);
        visualizer.drawConfiguration(config, filename, savePicture, camera);
    }

private:
    std::string getFilename(const std::string& path, unsigned long step){
        std::stringstream str;
        str << path << "/res/img";
        str << std::setw(4) << std::setfill('0') << step;
        return str.str();
    }

};

#endif //ROFI_ANIMATOR_H
