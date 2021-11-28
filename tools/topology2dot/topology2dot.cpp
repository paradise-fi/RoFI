#include <legacy/configuration/Configuration.h>
#include <legacy/configuration/IO.h>
#include <iostream>
#include <fstream>
#include <string>


std::string sTS(ShoeId s) {
    switch(s){
        case ShoeId::A:
            return "A";
        default:
            return "B";
    }
}

std::string dTS(ConnectorId d) {
    switch(d) {
        case ConnectorId::XPlus:
            return "X+";
        case ConnectorId::XMinus:
            return "X-";
        default:
            return "Z";
    }
}

std::string oTC(unsigned int ori) {
    switch(ori) {
        case 0:
            return "#396AB1";
        case 1:
            return "#DA7C30";
        case 2:
            return "#922428";
        default:
            return "#948B3D";
    }
}

std::string edgeToDot(Edge edge) {
    if (edge.id1() > edge.id2()) {
        edge = reverse(edge);
    }
    std::string head = std::to_string(edge.id1()) + " -- " + std::to_string(edge.id2());
    std::string headlabel = "headlabel=\"" + sTS(edge.side1()) + dTS(edge.dock1()) + "\",";
    std::string midlabel = "label=\"" + std::to_string(edge.ori()) + "\",";
    std::string taillabel = "taillabel=\"" + sTS(edge.side2()) + dTS(edge.dock2()) + "\",";
    std::string colour = "color=\"" + oTC(edge.ori()) + "\"";
    return "\t" + head + " [" + headlabel + midlabel + taillabel + colour + "];\n";
}


void drawDot(std::ofstream &output, const Configuration &cfg) {
    std::string dotGraph;

    dotGraph += "strict graph configuration {\n";

    for(const auto& [id, eList] : cfg.getEdges()){
        for(const auto& mEdge : eList)
            if(mEdge.has_value())
                dotGraph += edgeToDot(mEdge.value());
    }
    dotGraph += "}\n";
    output << dotGraph;
}


void printHelp() {
    std::cout << "Correct use:" << std::endl;
    std::cout << "\t./topology2dot [pathToInput] [?pathToOutput]" << std::endl;
}

int main(int argc, char* argv[]) {

    if (argc > 3 || argc == 1) {
        std::cout << "Incorrect use of tool." << std::endl;
        printHelp();
        return 1;
    }

    std::ifstream input;
    input.open(argv[1]);
    if(!input) {
        std::cout << "Could not open input file" << std::endl;
        return 1;
    }

    std::ofstream output;
    if(argc == 3) {
        output.open(argv[2]);
    } else {
        std::string outputPath(argv[1]);
        outputPath += ".dot";
        output.open(outputPath.c_str());
    }
    if(!output) {
        std::cout << "Could not open output file" << std::endl;
    }

    Configuration config;
    if(!IO::readConfiguration(input, config)) {
        std::cout << "Could not read config" << std::endl;
        return 1;
    }

    drawDot(output, config);
    return 0;
}
