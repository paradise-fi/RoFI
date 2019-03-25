//
// Created by maki on 13.3.19.
//

#ifndef ROFI_ANIMATOR_H
#define ROFI_ANIMATOR_H

#include "../Configuration.h"
#include "Generator.h"
#include "Visualizer.h"
#include <vector>
#include "../Printer.h"

class Animator{
private:
    static unsigned long outCount;
public:
    void visualizeMainConfigs(std::vector<Configuration>& mainConfigs, double maxPhi, unsigned int reconnectionPics,
            const std::string& path, bool savePicture, const Camera& cameraStart, const Camera& cameraEnd){
        bool firstConfig = true;
        if (mainConfigs.empty()){
            return;
        }
        if (mainConfigs.size() == 1){
            std::vector<Configuration> cfg{};
            cfg.push_back(mainConfigs.at(0));
            visualizeAllConfigs(cfg, path, savePicture, cameraStart, cameraEnd);
            return;
        }
        unsigned long allConfigsCount = getAllConfigsCount(mainConfigs, maxPhi, reconnectionPics);
        Camera currentCameraStart{};
        Camera currentCameraEnd{};
        unsigned long currentStep = 0;
        for (unsigned long i = 0; i < mainConfigs.size() - 1; i++){
            const Configuration& c1 = mainConfigs.at(i);
            const Configuration& c2 = mainConfigs.at(i + 1);
            std::vector<Configuration> generatedConfigs{};
            if (firstConfig){
                generatedConfigs.push_back(c1);
                firstConfig = false;
            }
            Generator generator;
            generator.generate(c1, c2, generatedConfigs, maxPhi, reconnectionPics);
            generatedConfigs.push_back(c2);
            currentCameraStart = countCameraMove(cameraStart, cameraEnd, currentStep, allConfigsCount - 1);
            currentCameraEnd = countCameraMove(cameraStart, cameraEnd, currentStep + generatedConfigs.size() - 1, allConfigsCount - 1);
            visualizeAllConfigs(generatedConfigs, path, savePicture, currentCameraStart, currentCameraEnd);
            currentStep += generatedConfigs.size();
        }

    }

    void visualizeAllConfigs(std::vector<Configuration>& allConfigs, const std::string& path, bool savePicture,
            Camera cameraStart, Camera cameraEnd){
        unsigned long step = 0;
        Configuration firstConfig = allConfigs.at(0);
        firstConfig.computeRotations();
        cameraStart.setCameraMassCenter(firstConfig.massCenter());
        cameraEnd.setCameraMassCenter(firstConfig.massCenter());
        for (Configuration& c : allConfigs){
            Camera camera = countCameraMove(cameraStart, cameraEnd, step, allConfigs.size() - 1);
            visualizeOneConfig(c, path, savePicture, camera);
            step++;
        }
    }

    void visualizeOneConfig(Configuration& config, const std::string& path, bool savePicture,
            const Camera& camera){
        Visualizer visualizer;
        config.computeRotations();
        std::string filename = getFilename(path);
        visualizer.drawConfiguration(config, filename, savePicture, camera);
    }

private:
    std::string getFilename(const std::string& path){
        std::stringstream str;
        str << path << "/img";
        str << std::setw(4) << std::setfill('0') << outCount++;
        return str.str();
    }

    unsigned int getAllConfigsCount(const std::vector<Configuration>& mainConfigs, double maxPhi,
            unsigned int reconnectionPics){
        if (mainConfigs.empty()){
          return 0;
        }
        if (mainConfigs.size() == 1){
          return 1;
        }
        unsigned int count = 1; //the very first configuration
        Generator generator;
        for (unsigned long i = 0; i < mainConfigs.size() - 1; i++){
            const Configuration& c1 = mainConfigs.at(i);
            const Configuration& c2 = mainConfigs.at(i + 1);
            count += generator.getStepsCount(c1, c2, maxPhi);
            if (c1.getEdges() != c2.getEdges()){
                unsigned int edgeChange = reconnectionPics / 2;
                count += edgeChange * 2;
                count += 1;     //c2 is not included
            }
        }
        return count;
    }

};

#endif //ROFI_ANIMATOR_H
