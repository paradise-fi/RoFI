//
// Created by xvozarov on 2/25/19.
//

#ifndef ROBOTS_PRINTER_H
#define ROBOTS_PRINTER_H

#include "Configuration.h"
#include <sstream>
#include <iomanip>

class Printer
{
public:
    std::string printMatrices(const Configuration& config)
    {
        std::stringstream out;
        for (const auto& [id, mod] : config.getModules())
        {
            out << "ID: " << id << ", side: A\n";
            out << print(mod.frameMatrix(A));
            out << "ID: " << id << ", side: B\n";
            out << print(mod.frameMatrix(B));
            out << std::endl;
        }
        return out.str();
    }

    std::string print(const std::vector<Configuration>& path)
    {
        std::stringstream out;
        for (const auto& config : path)
        {
            out << print(config) << std::endl;
        }
        return out.str();
    }

    std::string print(const Configuration& config)
    {
        std::stringstream out;
        for (const auto& [id, mod] : config.getModules())
        {
            out << print(mod);
        }
        for (const auto& [id, edges] : config.getEdges())
        {
            for (const Edge& edge : edges)
            {
                if (edge.getId1() < edge.getId2())
                {
                    out << print(edge);
                }
            }
        }
        return out.str();
    }


private:
    std::string print(const Matrix& matrix) const
    {
        std::stringstream out;
        out << std::fixed << std::setprecision(2);
        for (int i : {0,1,2,3})
        {
            for (int j : {0,1,2,3})
            {
                out << std::setw(7) << matrix(i,j);
            }
            out << std::endl;
        }

        return out.str();
    }

    std::string print(const Module& mod)
    {
        std::stringstream out;
        out << "M " <<
            mod.getId() << " " <<
            mod.getJoint(Alpha) << " " <<
            mod.getJoint(Beta) << " " <<
            mod.getJoint(Gamma) << std::endl;
        return out.str();
    }

    std::string print(const Edge& edge)
    {
        std::stringstream out;
        out << "E " <<
            edge.getId1() << " " <<
            edge.getSide1() << " " <<
            edge.getDock1() << " " <<
            edge.getOri() << " " <<
            edge.getDock2() << " " <<
            edge.getSide2() << " " <<
            edge.getId2() << std::endl;
        return out.str();
    }
};

#endif //ROBOTS_PRINTER_H
