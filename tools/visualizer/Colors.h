//
// Created by xnausova on 9/14/21.
//

#ifndef ROFI_COLORS_H
#define ROFI_COLORS_H

#include <vector>
#include <iostream>
#include <algorithm>

namespace {

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

enum RuleType { simple, multiple, range};

class ColorRule {
protected:
    std::array<int, 3> rgb;
    RuleType type;
    std::vector<int> multipleIds;
    int simpleId;
    int rangeIdFrom;
    int rangeIdTo;

public:
    ColorRule(int id, int red, int green, int blue){
        simpleId = id;
        type = simple;
        rgb = {red, green, blue};
    }

    ColorRule(int idFrom, int idTo, int red, int green, int blue){
        rangeIdFrom = idFrom;
        rangeIdTo = idTo;
        type = range;
        rgb = {red, green, blue};
    }

    ColorRule(const std::vector<int> &idsVec, int red, int green, int blue) {
        multipleIds = idsVec;
        type = multiple;
        rgb = {red, green, blue};
    }

    bool applyRule(int otherId) const {
        switch(type)
        {
            case simple:
                return simpleId == otherId;
            case multiple:
                return std::find(multipleIds.begin(), multipleIds.end(), otherId) != multipleIds.end();
            case range:
                return otherId >= rangeIdFrom && otherId <= rangeIdTo;
            default:
                return false;
        }
    }

    std::array<int, 3> getRGB() const {
        return rgb;
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
        std::string::size_type toPosition = idsStr.find(',');
        std::string::size_type fromPosition = 0;
        while (toPosition != std::string::npos){
            std::string idStr = idsStr.substr(fromPosition, toPosition);
            int id = std::stoi(idStr);
            ids.push_back(id);
            fromPosition = ++toPosition;
            toPosition = idsStr.find(',', fromPosition);
        }
        std::string idStr = idsStr.substr(fromPosition, toPosition);
        int id = std::stoi(idStr);
        ids.push_back(id);
        fromPosition = ++toPosition;
        toPosition = idsStr.find(',', fromPosition);
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
            if (type.empty()){
                continue;
            }
            if (type == "S") {          //simple color rule
                int id;
                str >> id;
                std::array<int, 3> color;
                readColor(str, line, color);
                ColorRule rule(id, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else if (type == "R") {   //range color rule
                int idFrom, idTo;
                str >> idFrom >> idTo;
                std::array<int, 3> color;
                readColor(str, line, color);
                ColorRule rule(idFrom, idTo, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else if (type == "M") {   //multiple color rule
                std::string idsStr;
                str >> idsStr;
                std::vector<int> ids;
                readMultipleIds(idsStr, ids);
                std::array<int, 3> color;
                readColor(str, line, color);
                ColorRule rule(ids, color[0], color[1], color[2]);
                colorRules.push_back(rule);
            } else {
                throw std::runtime_error(
                        "Expected color settings (S = single, M = multiple, R = range), got " + type + ".");
            }
        }
    }


} // namespace IO

} // namespace

#endif //ROFI_COLORS_H
