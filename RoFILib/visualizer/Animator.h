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
private:
    unsigned long outCount = 0;
public:
    void visualizeMainConfigs(std::vector<Configuration>& mainConfigs, double maxPhi, unsigned int reconnectionPics,
            const std::string& path, bool savePicture, Camera cameraStart, Camera cameraEnd,
            const Resolution& resolution, int magnify){
        bool firstConfig = true;
        if (mainConfigs.empty()){
            return;
        }
        if (mainConfigs.size() == 1){
            std::vector<Configuration> cfg;
            cfg.push_back(mainConfigs[0]);
            visualizeAllConfigs(cfg, path, savePicture, cameraStart, cameraEnd, resolution, magnify);
            return;
        }
        unsigned long allConfigsCount = getAllConfigsCount(mainConfigs, maxPhi, reconnectionPics);
        Camera currentCameraStart;
        Camera currentCameraEnd;
        unsigned long currentStep = 0;
        setCamerasDefault(mainConfigs.at(0), cameraStart, cameraEnd);
        for (unsigned long i = 0; i < mainConfigs.size() - 1; i++){
            const Configuration& c1 = mainConfigs.at(i);
            const Configuration& c2 = mainConfigs.at(i + 1);
            std::vector<Configuration> generatedConfigs;
            if (firstConfig){
                generatedConfigs.push_back(c1);
                firstConfig = false;
            }
            Generator generator;
            generator.generate(c1, c2, generatedConfigs, maxPhi, reconnectionPics);
            generatedConfigs.push_back(c2);
            currentCameraStart = interpolateCamera(cameraStart, cameraEnd, currentStep, allConfigsCount - 1);
            currentCameraEnd = interpolateCamera(cameraStart, cameraEnd, currentStep + generatedConfigs.size() - 1, allConfigsCount - 1);
            visualizeAllConfigs(generatedConfigs, path, savePicture, currentCameraStart, currentCameraEnd,
                    resolution, magnify);
            currentStep += generatedConfigs.size();
        }
    }

    void visualizeAllConfigs(std::vector<Configuration>& allConfigs, const std::string& path, bool savePicture,
            Camera cameraStart, Camera cameraEnd, const Resolution& resolution,
                             int magnify){
        unsigned long step = 0;
        setCamerasDefault(allConfigs.at(0), cameraStart, cameraEnd);
        for (Configuration& c : allConfigs){
            Camera camera = interpolateCamera(cameraStart, cameraEnd, step, allConfigs.size() - 1);
            visualizeOneConfig(c, path, savePicture, camera, resolution, magnify);
            step++;
        }
    }

    void visualizeOneConfig(Configuration& config, const std::string& path, bool savePicture,
            const Camera& camera, const Resolution& resolution,
            int magnify){
        Visualizer visualizer;
        config.computeMatrices();
        std::string filename = getFilename(path);
        visualizer.drawConfiguration(config, filename, savePicture, camera, resolution, magnify);
    }

private:
    void setCamerasDefault(Configuration config, Camera& cameraStart, Camera& cameraEnd){
        config.computeMatrices();
        cameraStart.setCameraMassCenter(config.massCenter());
        cameraEnd.setCameraMassCenter(config.massCenter());
    }

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
            unsigned int moduleChangeCount = generator.getStepsCount(c1, c2, maxPhi);
            unsigned int currentCount = moduleChangeCount;
            if (c1.getEdges() != c2.getEdges()){
                currentCount = moduleChangeCount > reconnectionPics ? moduleChangeCount : reconnectionPics;
            }
            count += currentCount;
        }
        return count;
    }
};

#endif //ROFI_ANIMATOR_H
