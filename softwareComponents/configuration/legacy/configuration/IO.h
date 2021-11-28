//
// Created by xvozarov on 5/5/19.
//

#ifndef ROFI_IO_H
#define ROFI_IO_H

#include "Configuration.h"
#include <sstream>
#include <iomanip>

namespace {
    using namespace rofi::configuration::matrices;

namespace IO
{

    const std::string sideName[] = {"A", "B"};
    const std::string sideNum[] = {"0", "1"};
    const std::string conName[] = {"+X", "-X", "-Z"};
    const std::string conNum[] = {"0", "1", "2"};
    const std::string oriName[] = {"N", "E", "S", "W"};
    const std::string oriNum[] = {"0", "1", "2", "3"};

/* TO STRING methods
 * For creating configuration files.
 * */

// Attributes

    inline std::string toString(ConnectorId d)
    {
        return conName[d];
    }

    inline std::string toString(ShoeId s)
    {
        return sideName[s];
    }

    inline std::string toString(unsigned o)
    {
        return oriName[o];
    }

// Module

    inline std::string toString(const Module &mod)
    {
        std::stringstream out;
        out << mod.getId();
        for (auto j : {Alpha, Beta, Gamma})
            out << " " << mod.getJoint(j);
        return out.str();
    }

// Edge

    inline std::string toString(const Edge &edge)
    {
        std::stringstream out;
        out << edge.id1() << " " <<
            toString(edge.side1()) << " " <<
            toString(edge.dock1()) << " " <<
            toString(edge.ori()) << " " <<
            toString(edge.dock2()) << " " <<
            toString(edge.side2()) << " " <<
            edge.id2();
        return out.str();
    }

// Configuration

    inline std::string toString(const Configuration &cfg)
    {
        std::stringstream out;
        out << "C\n";
        for (const auto&[id, mod] : cfg.getModules()) {
            out << "M " << toString(mod) << std::endl;
        }
        for (const auto&[id, edges] : cfg.getEdges()) {
            for (const std::optional< Edge > &edgeOpt : edges) {
                if (!edgeOpt.has_value())
                    continue;
                const Edge &edge = edgeOpt.value();
                if (edge.id1() < edge.id2()) {
                    out << "E " << toString(edge) << std::endl;
                }
            }
        }
        return out.str();
    }

    inline std::string toString(const std::vector< Configuration > &path)
    {
        std::stringstream out;
        for (const auto &config : path) {
            out << toString(config) << std::endl;
        }
        return out.str();
    }

// Actions

    inline std::string toString(const Action::Rotate &rotation)
    {
        std::stringstream out;

        out << "R " <<
            rotation.id() << " " <<
            rotation.joint() << " " <<
            rotation.angle() << std::endl;

        return out.str();
    }

    inline std::string toString(const Action::Reconnect &reconnection)
    {
        std::stringstream out;
        out << (reconnection.add() ? "C " : "D ") <<
            toString(reconnection.edge()) << std::endl;

        return out.str();
    }

    inline std::string toString(const Action &action)
    {
        std::stringstream out;
        for (const auto &rotation : action.rotations()) {
            out << toString(rotation);
        }

        for (const auto &reconnection : action.reconnections()) {
            out << toString(reconnection);
        }

        return out.str();
    }

    inline std::string toString(const std::vector< Action > &actions)
    {
        std::stringstream out;
        for (const Action &action : actions) {
            out << toString(action) << std::endl;
        }

        return out.str();
    }

/* FROM STRING methods
 * For reading and parsing configuration files.
 * */

// Reading attributes and edges.

    inline ConnectorId readCon(const std::string &in)
    {
        for (unsigned i = 0; i < 3; ++i) {
            if (in == conName[i])
                return static_cast<ConnectorId>(i);
            if (in == conNum[i])
                return static_cast<ConnectorId>(i);
        }
        throw std::runtime_error("Expected connector (+X, -X, -Z), got " + in + ".");
    }

    inline unsigned readOri(const std::string &in)
    {
        for (unsigned i = 0; i < 4; ++i) {
            if (in == oriName[i])
                return i;
            if (in == oriNum[i])
                return i;
        }
        throw std::runtime_error("Expected orientation (N, E, S, W), got " + in + ".");
    }

    inline ShoeId readSide(const std::string &in)
    {
        for (unsigned i = 0; i < 2; ++i) {
            if (in == sideName[i])
                return static_cast<ShoeId>(i);
            if (in == sideNum[i])
                return static_cast<ShoeId>(i);
        }
        throw std::runtime_error("Expected side (A, B), got " + in + ".");
    }

    inline Edge readEdge(std::istringstream &tmp)
    {
        ID id1, id2;
        std::string s1, p1, ori, p2, s2;
        tmp >> id1 >> s1 >> p1 >> ori >> p2 >> s2 >> id2;
        return {id1, readSide(s1), readCon(p1), readOri(ori), readCon(p2), readSide(s2), id2};
    }

// Reading configurations

    inline bool readConfiguration(std::istream &input, Configuration &cfg)
    {
        std::string s;
        std::streampos lineStart;
        bool first = true;

        while (lineStart = input.tellg(), getline(input, s)) {
            std::string type;
            std::istringstream tmp(s);
            tmp >> type;
            if (type.empty() || type[0] == '#' ) {
                continue; // Empty line or a comment
            }

            if (type == "C") {
                if ( !first ) {
                    // Next configuration found, revert last line
                    input.seekg(lineStart);
                    return !cfg.empty();
                }
                first = false;
                continue;
            }

            if ( first )
                throw std::runtime_error("Configuration does not start with command 'C'");

            if (type == "M") {
                double alpha, beta, gamma;
                unsigned int id;
                tmp >> id >> alpha >> beta >> gamma;
                cfg.addModule(alpha, beta, gamma, id);
            }
            else if (type == "E") {
                cfg.addEdge(readEdge(tmp));
            }
            else {
                throw std::runtime_error("Expected side module (M) or edge (E), got " + type + ".");
            }
        }

        return !cfg.empty();
    }

    inline void readConfigurations(std::istream &input, std::vector< Configuration > &configs)
    {
        Configuration config;
        while (readConfiguration(input, config)) {
            configs.push_back(config);
            config = Configuration();
        }
    }

// Reading actions

    inline Action readAction(std::istream &input)
    {
        std::string s;
        std::vector< Action::Rotate > rotations;
        std::vector< Action::Reconnect > reconnections;

        while (getline(input, s)) {
            if (s.empty()) {
                break;
            }

            char type;
            std::istringstream tmp(s);
            tmp >> type;
            if (type == 'R') {
                unsigned int id, joint;
                double angle;
                tmp >> id >> joint >> angle;
                rotations.emplace_back(id, static_cast<Joint>(joint), angle);
            } else if (type == 'D' || type == 'C') {
                reconnections.emplace_back(type == 'C', readEdge(tmp));
            }
        }

        return {rotations, reconnections};
    }

/* MATRIX TO STRING
 * Mostly for debug purposes */

    inline std::string toString(const Matrix &matrix)
    {
        std::stringstream out;
        out << std::fixed << std::setprecision(2);
        for (int i : {0, 1, 2, 3}) {
            for (int j : {0, 1, 2, 3}) {
                out << std::setw(7) << matrix(i, j);
            }
            out << std::endl;
        }

        return out.str();
    }

    inline std::string toStringMatrices(const Configuration &config)
    {
        std::stringstream out;
        for (const auto&[id, ms] : config.getMatrices()) {
            out << "ID: " << id << ", side: A\n";
            out << toString(ms[A]);
            out << "ID: " << id << ", side: B\n";
            out << toString(ms[B]);
            out << std::endl;
        }
        return out.str();
    }

}

} // namespace

#endif //ROFI_IO_H
