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
            out << print(mod.shoeMatrix(A));
            out << "ID: " << id << ", side: B\n";
            out << print(mod.shoeMatrix(B));
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
            for (const std::optional<Edge>& edgeOpt : edges)
            {
                if (!edgeOpt.has_value())
                    continue;
                const Edge& edge = edgeOpt.value();
                if (edge.id1 < edge.id2)
                {
                    out << print(edge);
                }
            }
        }
        return out.str();
    }

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
            edge.id1 << " " <<
            edge.side1 << " " <<
            edge.dock1 << " " <<
            edge.ori << " " <<
            edge.dock2 << " " <<
            edge.side2 << " " <<
            edge.id2 << std::endl;
        return out.str();
    }
};

#endif //ROBOTS_PRINTER_H
