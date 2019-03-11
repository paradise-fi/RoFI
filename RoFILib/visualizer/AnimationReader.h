//
// Created by maki on 11.3.19.
//

#ifndef ROFI_ANIMATIONREADER_H
#define ROFI_ANIMATIONREADER_H

#include <fstream>
#include "../Reader.h"
#include "Visualizer.h"
#include "Generator.h"

class AnimationReader{
public:
    void read(const std::string& inputName){
        unsigned long position = inputName.find_last_of('/');
        std::string path = inputName.substr(0, position);

        std::ifstream seqFile;
        seqFile.open(inputName);

        std::string line;
        std::string filename_1;
        std::string filename_2;
        std::ifstream file_1;
        std::ifstream file_2;
        unsigned int steps;
        std::vector<Configuration> configs;

        std::getline(seqFile, line);

        Camera cameraStart, cameraEnd;
        bool cameraMove = false;
        while(readCameraSettings(cameraStart, cameraEnd, cameraMove, line)){
            std::getline(seqFile, line);
        }

        filename_2 = path + "/" + line;
        bool firstFile = true;

        while(std::getline(seqFile, line)){
            if (line.empty()){
                break;
            }
            std::stringstream tmp(line);
            tmp >> steps;
            filename_1 = filename_2;
            std::getline(seqFile, line);
            filename_2 = path + "/" + line;
            file_1.open(filename_1);
            file_2.open(filename_2);

            Reader reader;
            Configuration c1, c2;
            reader.read(file_1, c1);
            reader.read(file_2, c2);
            if (firstFile){
                configs.push_back(c1);
                firstFile = false;
            }
            if (steps > 0) {
                Generator generator;
                std::vector<Configuration> interConfigs;
                generator.generate(c1, c2, interConfigs, steps);
                for (const Configuration& c : interConfigs){
                    configs.push_back(c);
                }
            }
            configs.push_back(c2);
            file_1.close();
            file_2.close();
        }

        unsigned long configCount = configs.size();
        unsigned long step = 0;
        Camera cameraResult;
        for (Configuration& c : configs){
            if (!cameraMove){
                cameraResult = cameraStart;
            } else {
                cameraResult = countCameraMove(cameraStart, cameraEnd, step, configCount - 1);
            }
            visualize(c, path, step, cameraResult);
            step++;
        }
    }


private:
    void visualize(Configuration& config, const std::string& path, unsigned long step, const Camera& camera){
        Visualizer visualizer;
        config.computeRotations();
        std::string filename = getFilename(path, step);
        visualizer.drawConfiguration(config, filename, true, camera);
    }

    std::string getFilename(const std::string& path, unsigned long step){
        std::stringstream str;
        str << path << "/res/img";
        str << std::setw(3) << std::setfill('0') << step;
        return str.str();
    }



    bool readCameraSettings(Camera& cameraStart, Camera& cameraEnd, bool& cameraMove, const std::string& line){
        if(line[0] != 'C'){     //not for camera settings
            return false;
        }
        std::stringstream str(line);
        std::string type;
        str >> type;
        int xs, ys, zs, xe, ye, ze;
        if (type == "CP"){          //camera position
            str >> xs >> ys >> zs;
            cameraStart.setPos(xs, ys, zs);
            cameraEnd.setPos(xs, ys, zs);
        } else if (type == "CV"){   //camera viewUp
            str >> xs >> ys >> zs;
            cameraStart.setView(xs, ys, zs);
            cameraEnd.setView(xs, ys, zs);
        } else if (type == "CF"){   //camera focal point
            str >> xs >> ys >> zs;
            cameraStart.setFoc(xs, ys, zs);
            cameraEnd.setFoc(xs, ys, zs);
        } else if (type == "CPM"){  //camera position move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setPos(xs, ys, zs);
            cameraEnd.setPos(xe, ye, ze);
            cameraMove = true;
        } else if (type == "CVM"){  //camera viewUp move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setView(xs, ys, zs);
            cameraEnd.setView(xe, ye, ze);
            cameraMove = true;
        } else if (type == "CFM"){  //camera focal point move
            str >> xs >> xe >> ys >> ye >> zs >> ze;
            cameraStart.setFoc(xs, ys, zs);
            cameraEnd.setFoc(xe, ye, ze);
            cameraMove = true;
        } else {
            return false;
        }
        return true;
    }

    double countStep(double a, double b, unsigned long step, unsigned long totalSteps){
        return a + (((b - a) * step) / totalSteps);
    }

    void setPosition(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
            unsigned long step, unsigned long totalSteps){
        res.setPosX(countStep(cameraStart.getPosX(), cameraEnd.getPosX(), step, totalSteps));
        res.setPosY(countStep(cameraStart.getPosY(), cameraEnd.getPosY(), step, totalSteps));
        res.setPosZ(countStep(cameraStart.getPosZ(), cameraEnd.getPosZ(), step, totalSteps));
    }

    void setView(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
                     unsigned long step, unsigned long totalSteps){
        res.setViewX(countStep(cameraStart.getViewX(), cameraEnd.getViewX(), step, totalSteps));
        res.setViewY(countStep(cameraStart.getViewY(), cameraEnd.getViewY(), step, totalSteps));
        res.setViewZ(countStep(cameraStart.getViewZ(), cameraEnd.getViewZ(), step, totalSteps));
    }

    void setFocus(const Camera& cameraStart, const Camera& cameraEnd, Camera& res,
                     unsigned long step, unsigned long totalSteps){
        res.setFocX(countStep(cameraStart.getFocX(), cameraEnd.getFocX(), step, totalSteps));
        res.setFocY(countStep(cameraStart.getFocY(), cameraEnd.getFocY(), step, totalSteps));
        res.setFocZ(countStep(cameraStart.getFocZ(), cameraEnd.getFocZ(), step, totalSteps));
    }

    Camera countCameraMove(const Camera& cameraStart, const Camera& cameraEnd,
            unsigned long step, unsigned long totalSteps){
        Camera res;
        setPosition(cameraStart, cameraEnd, res, step, totalSteps);
        setView(cameraStart, cameraEnd, res, step, totalSteps);
        setFocus(cameraStart, cameraEnd, res, step, totalSteps);
        return res;
    }


};

#endif //ROFI_ANIMATIONREADER_H
