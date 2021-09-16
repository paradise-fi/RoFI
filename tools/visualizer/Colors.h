//
// Created by xnausova on 9/14/21.
//

#ifndef ROFI_COLORS_H
#define ROFI_COLORS_H

#include <vector>
#include <iostream>
#include <algorithm>

const int COLORS[10][3] = { {255, 255, 255},
                            {0, 255, 0},
                            {0, 0, 255},
                            {191, 218, 112},
                            {242, 202, 121},
                            {218, 152, 207},
                            {142, 202, 222},
                            {104, 135, 205},
                            {250, 176, 162},
                            {234, 110, 111}};



class ColorRule {
protected:
    std::array<int, 3> rgb;

public:
    virtual ~ColorRule() {}
    virtual bool applyRule(int otherId) const {
        return false;
    }

    std::array<int, 3> getRGB() const {
        return rgb;
    }
};



class SimpleColorRule : public ColorRule {
    int id;

public:
    SimpleColorRule(int id, int red, int green, int blue) {
        id = id;
        rgb = {red, green, blue};
    }

    bool applyRule(int otherId) const override{
        if (id == otherId) {
            return true;
        }
        return false;
    }

};


class MultipleColorRule : public ColorRule {
    std::vector<int> ids;

public:
    MultipleColorRule(const std::vector<int> &idsVec, int red, int green, int blue) {
        ids = idsVec; //TODO copy?
        rgb = {red, green, blue};
    }

    bool applyRule(int otherId) const override{
        if (std::find(ids.begin(), ids.end(), otherId) != ids.end()) {
            return true;
        }
        return false;
    }

};


class RangeColorRule : public ColorRule {
    int idFrom;
    int idTo;

public:
    RangeColorRule(int idFrom, int idTo, int red, int green, int blue) {
        idFrom = idFrom;
        idTo = idTo;
        rgb = {red, green, blue};
    }

    bool applyRule(int otherId) const override{
        if (otherId >= idFrom && otherId <= idTo) {
            return true;
        }
        return false;
    }
};


inline std::array<int, 3> getColor(int id, std::vector<ColorRule> colorRules){

    // default color by id
    std::array<int, 3> color;
    color[0] = COLORS[id % 7 + 3][0];
    color[1] = COLORS[id % 7 + 3][1];
    color[2] = COLORS[id % 7 + 3][2];

    for (const ColorRule &rule : colorRules) {
        if (rule.applyRule(id)) {
            color = rule.getRGB();
        }
    }
    return color;
}


namespace IO {

    inline void readMultipleIds(std::string idsStr, std::vector<int> &ids){
        std::string::size_type position = idsStr.find(',');
        while (position != std::string::npos){
            std::string idStr = idsStr.substr(0, position);
            int id = std::stoi(idStr);
            ids.push_back(id);
            position = idsStr.find(',', position);
        }
    }

    inline void readColor(std::stringstream &str, const std::string &line, std::array<int, 3> &color){
        if (line.find("#") != std::string::npos) {
            int colorId;
            std::string colorIdStr;
            str >> colorIdStr;
            colorIdStr.erase(0, 1);
            colorId = std::stoi(colorIdStr);
            color[0] = COLORS[colorId % 7 + 3][0];
            color[1] = COLORS[colorId % 7 + 3][1];
            color[2] = COLORS[colorId % 7 + 3][2];
        } else {
            int r, g, b;
            str >> r >> g >> b;
            color[0] = r;
            color[1] = g;
            color[2] = b;
        }
    }

    inline void readColorRules(std::istream &input, std::vector<ColorRule> &colorRules) {
        std::string line;
        while (getline(input, line)) {
            std::stringstream str(line);
            std::string type;
            str >> type;
            if (type == "S") {          //simple color rule
                int id;
                str >> id;
                std::array<int, 3> color;
                readColor(str, line, color);
                SimpleColorRule rule(id, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else if (type == "R") {   //range color rule
                int idFrom, idTo;
                str >> idFrom >> idTo;
                std::array<int, 3> color;
                readColor(str, line, color);
                RangeColorRule rule(idFrom, idTo, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else if (type == "M") {   //multiple color rule
                std::string idsStr;
                str >> idsStr;
                std::vector<int> ids;
                readMultipleIds(idsStr, ids);
                std::array<int, 3> color;
                readColor(str, line, color);
                MultipleColorRule rule(ids, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else {
                throw std::runtime_error(
                        "Expected color settings (S = single, M = multiple, R = range), got " + type + ".");
            }
        }
    }





} // namespace IO

#endif //ROFI_COLORS_H
