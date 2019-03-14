//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_READER_H
#define ROBOTS_READER_H

#include <istream>
#include "Configuration.h"
#include "visualizer/Camera.h"

class Reader
{
public:
    bool read(std::istream& input, Configuration& config)
    {
        std::string s;

        while (getline(input, s))
        {
            if (s.empty())
            {
                break;
            }
            char type;
            std::stringstream tmp(s);
            tmp >> type;
            if (type == 'M')
            {
                double alpha, beta, gamma;
                unsigned int id;
                tmp >> id >> alpha >> beta >> gamma;
                config.addModule(alpha, beta, gamma, id);
            }
            if (type == 'E')
            {
                unsigned int id1, s1, p1, ori, p2, s2, id2;
                tmp >> id1 >> s1 >> p1 >> ori >> p2 >> s2 >> id2;
                config.addEdge({id1, static_cast<Side>(s1), static_cast<Dock>(p1), ori, static_cast<Dock>(p2), static_cast<Side>(s2), id2});
            }
        }
        return !config.empty();
    }

    void read(std::istream& input, std::vector<Configuration>& configs)
    {
        Configuration config;
        while (read(input, config))
        {
            configs.push_back(config);
            config = Configuration();
        }
    }

    void readCameraSettings(std::istream& input, Camera& cameraStart, Camera& cameraEnd, bool& cameraMove){
        std::string line;
        while(getline(input, line)) {
            if (line[0] != 'C') {     //not for camera settings
                continue;
            }
            std::stringstream str(line);
            std::string type;
            str >> type;
            int xs, ys, zs, xe, ye, ze;
            if (type == "CP") {          //camera position
                str >> xs >> ys >> zs;
                cameraStart.setPos(xs, ys, zs);
                cameraEnd.setPos(xs, ys, zs);
            } else if (type == "CV") {   //camera viewUp
                str >> xs >> ys >> zs;
                cameraStart.setView(xs, ys, zs);
                cameraEnd.setView(xs, ys, zs);
            } else if (type == "CF") {   //camera focal point
                str >> xs >> ys >> zs;
                cameraStart.setFoc(xs, ys, zs);
                cameraEnd.setFoc(xs, ys, zs);
            } else if (type == "CPM") {  //camera position move
                str >> xs >> xe >> ys >> ye >> zs >> ze;
                cameraStart.setPos(xs, ys, zs);
                cameraEnd.setPos(xe, ye, ze);
                cameraMove = true;
            } else if (type == "CVM") {  //camera viewUp move
                str >> xs >> xe >> ys >> ye >> zs >> ze;
                cameraStart.setView(xs, ys, zs);
                cameraEnd.setView(xe, ye, ze);
                cameraMove = true;
            } else if (type == "CFM") {  //camera focal point move
                str >> xs >> xe >> ys >> ye >> zs >> ze;
                cameraStart.setFoc(xs, ys, zs);
                cameraEnd.setFoc(xe, ye, ze);
                cameraMove = true;
            }
        }
    }
};

#endif //ROBOTS_READER_H
