//
// Created by xvozarov on 2/18/19.
//

#ifndef ROBOTS_READER_H
#define ROBOTS_READER_H

#include <istream>
#include "Configuration.h"

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
                config.addEdge(id1, static_cast<Side>(s1), static_cast<Dock>(p1), ori, static_cast<Dock>(p2), static_cast<Side>(s2), id2);
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
};

#endif //ROBOTS_READER_H
