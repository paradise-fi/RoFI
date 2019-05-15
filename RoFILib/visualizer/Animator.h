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
    /**
     * This function is used for visualization of main configs. This function
     * esures, that more configurations are generated.
     *
     * @param mainConfigs main configurations to be visualized
     * @param maxPhi maximal angle difference in one step
     * @param reconnectionPics number of steps for reconnection
     * @param path where to save the pictures
     * @param savePicture whether to save pictures or render the configurations on the screen
     * @param cameraStart camera parameters at the beginning
     * @param cameraEnd camera parameters at the end
     * @param resolution size of the renderer window
     * @param magnify increase the resolution of the saved image
     */
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

    /**
     * This function is used for visualization of configurations.
     *
     * @param allConfigs configurations to be visualized
     * @param path where to save the pictures
     * @param savePicture whether to save pictures or render the configurations on the screen
     * @param cameraStart camera parameters at the beginning
     * @param cameraEnd camera parameters at the end
     * @param resolution size of the renderer window
     * @param magnify increase the resolution of the saved image
     */
    void visualizeAllConfigs(std::vector<Configuration>& allConfigs, const std::string& path, bool savePicture,
            Camera cameraStart, Camera cameraEnd, const Resolution& resolution,
                             int magnify){
        unsigned long step = 0;
        setCamerasDefault(allConfigs.at(0), cameraStart, cameraEnd);
        Camera camera;
        bool cameraMove = true;
        if (cameraStart == cameraEnd){
            camera = cameraStart;
            cameraMove = false;
        }
        for (Configuration& c : allConfigs){
            if (cameraMove) {
                camera = interpolateCamera(cameraStart, cameraEnd, step, allConfigs.size() - 1);
            }
            visualizeOneConfig(c, path, savePicture, camera, resolution, magnify);
            step++;
        }
    }

    /**
     * This function is used for visualization of one configuration.
     *
     * @param config configuration to be visualized
     * @param path where to save the pictures
     * @param savePicture whether to save pictures or render the configurations on the screen
     * @param camera camera parameters
     * @param resolution size of the renderer window
     * @param magnify increase the resolution of the saved image
     */
    void visualizeOneConfig(Configuration& config, const std::string& path, bool savePicture,
            const Camera& camera, const Resolution& resolution,
            int magnify){
        Visualizer visualizer;
        config.computeMatrices();
        std::string filename = getFilename(path);
        visualizer.drawConfiguration(config, filename, savePicture, camera, resolution, magnify);
    }

private:
    /**
     * This function sets cameras default parameters to
     * parameters dependent on mass center of the configuration.
     *
     * @param config
     * @param cameraStart
     * @param cameraEnd
     */
    void setCamerasDefault(Configuration config, Camera& cameraStart, Camera& cameraEnd){
        config.computeMatrices();
        cameraStart.setCameraMassCenter(config.massCenter());
        cameraEnd.setCameraMassCenter(config.massCenter());
    }

    /**
     * This function creates a unique filename.
     *
     * @param path where to save the pictures
     * @return unique filename
     */
    std::string getFilename(const std::string& path){
        std::stringstream str;
        str << path << "/img";
        str << std::setw(4) << std::setfill('0') << outCount++;
        return str.str();
    }

    /**
     * This function returns count of all configs to be visualized.
     *
     * @param mainConfigs main configuration
     * @param maxPhi maximal angle difference in one step
     * @param reconnectionPics number of steps for reconnection
     * @return number of all configs
     */
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
