#include <iostream>
#include <fstream>
#include <string_view>
#include "Visualizer.h"
#include "Animator.h"
#include <cxxopts.hpp>
#include <legacy/configuration/IO.h>

using namespace IO;
//using Resolution = std::pair<int, int>;

using namespace rofi::configuration::matrices;

struct Parameters{
    bool many = false;
    bool savePicture = false;
    bool animation = false;
    std::ifstream inputFile;
    std::ifstream cameraSettings;
    bool cameraSet = false;
    std::string path = "../data/res";
    double omega = 120;
    double phi = 5;
    double reconnectionTime = 2;
    unsigned int reconnectionPics = 48;
    unsigned int framerate = 24;
    Resolution resolution = {1920, 1080};
    int magnify = 1;
    std::ifstream colorsFile;
    bool colorsSet = false;
};

namespace err {
    std::string notAvailable(const std::string &option, const std::string &condition) {
        std::stringstream str;
        str << "The option " << option << " is available only with the option " << condition << ".\n";
        return str.str();
    }

    std::string atMostOne(const std::string &option) {
        std::stringstream str;
        str << "There can be at most one " << option << " option.\n";
        return str.str();
    }

    std::string exactlyOne(const std::string &option) {
        std::stringstream str;
        str << "There must be exactly one " << option << " option.\n";
        return str.str();
    }
}//namespace err

void parse(int argc, char* argv[], Parameters& p){
    cxxopts::Options options("rofi-vis", "RoFI Visualizer: Tool for visualization of configurations and creating animations.");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("i,input", "Input config file", cxxopts::value<std::string>())
            ("s,save", "Save picture to file")
            ("a,animation", "Create animation from configurations")
            ("c,camera", "Camera settings file", cxxopts::value<std::string>())
            ("p,path", "Path where to save pictures", cxxopts::value<std::string>())
            ("f,framerate", "Number of pictures per second", cxxopts::value<unsigned int>())
            ("v,velocity", "Maximal angular velocity in 1°/s", cxxopts::value<double>())
            ("g,angle", "Maximal angle diff in ° per picture", cxxopts::value<double>())
            ("t,recTime", "Time in seconds for reconnection", cxxopts::value<double>())
            ("e,recPics", "Number of pictures for reconnection", cxxopts::value<unsigned int>()) //rename
            ("n,many", "Many configurations in one file")
            ("r,resolution", "Size of the window on the screen or resolution of the saved picture in format "
                             "<num>x<num>", cxxopts::value<std::string>())
            ("m,magnify", "Magnification of saved pictures", cxxopts::value<int>())
            ("q,color", "Colors definition file", cxxopts::value<std::string>())
            ;

    try {
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        if (result.count("input") == 1){
            std::string filename(result["input"].as< std::string >());
            p.inputFile.open(filename);
            if (!p.inputFile.good()){
                std::cerr << "Could not open file " << filename << ".\n";
                exit(1);
            }
        } else {
            std::cerr << err::exactlyOne("'-i' or '--input'");
            exit(1);
        }

        if (result.count("save")){
            p.savePicture = true;
        }

        if (result.count("animation")){
            p.animation = true;
            p.many = true;
        }

        if (result.count("camera") == 1){
            std::string filename = result["camera"].as< std::string >();
            p.cameraSettings.open(filename);
            if (!p.cameraSettings.good()) {
                std::cerr << "Could not open file " << filename << ".\n";
                exit(1);
            }
            p.cameraSet = true;
        } else if (result.count("camera") > 1) {
            std::cerr << err::atMostOne("'-c' or '--camera'");
            exit(1);
        }

        if (result.count("color") == 1){
            std::string colorsFilename = result["color"].as< std::string >();
            p.colorsFile.open(colorsFilename);
            if (!p.colorsFile.good()){
                std::cerr << "Could not open file " << colorsFilename << ".\n";
                exit(1);
            }
            p.colorsSet = true;
        } else if (result.count("color") > 1) {
            std::cerr << err::atMostOne("'-q' or '--color'");
            exit(1);
        }

        if (result.count("path") == 1){
            p.path = result["path"].as< std::string >();
            if (!result.count("save")){
                std::cerr << "Path is not used because '-s' or '--save' is not set.\n";
            }
        } else if (result.count("path") > 1){
            std::cerr << err::atMostOne("'-p' or '--path'");
            exit(1);
        }

        if (result.count("framerate") == 1){
            if (!p.animation){
                std::cerr << err::notAvailable("'-f' or '--framerate'", "'-a' or '--animation'");
                exit(1);
            }
            p.framerate = result["framerate"].as<unsigned int>();
            p.phi = p.omega / p.framerate;
            p.reconnectionPics = static_cast<unsigned int>(std::ceil(p.reconnectionTime * p.framerate));
        } else if (result.count("framerate") > 1){
            std::cerr << err::atMostOne("'-f' or '--framerate'");
            exit(1);
        }

        if (result.count("velocity") == 1 && result.count("angle") == 0){
            if (!p.animation){
                std::cerr << err::notAvailable("'-v' or '--velocity'", "'-a' or '--animation'");
                exit(1);
            }
            double val = result["velocity"].as< double >();
            if (val < 0) {
                std::cerr << "Angular velocity can not be negative.\n";
                exit(1);
            }
            p.omega = val;
            p.phi = p.omega / p.framerate;
        } else if (result.count("velocity") == 0 && result.count("angle") == 1) {
            if (!p.animation){
                std::cerr << err::notAvailable("'-g' or '--angle'", "'-a' or '--animation'");
                exit(1);
            }
            double val = result["angle"].as< double >();
            if (val < 0) {
                std::cerr << "Maximal angle diff can not be negative.\n";
                exit(1);
            }
            p.phi = val;
            p.omega = p.framerate * p.phi;
        } else if (!result.count("velocity") && !result.count("angle")){}
        else {
            std::cerr << err::atMostOne("'-v', '--velocity', '-g' or '--angle'");
            exit(1);
        }

        if (result.count("recTime") == 1 && result.count("recPics") == 0){
            if (!p.animation){
                std::cerr << err::notAvailable("'-t' or '--recTime'", "'-a' or '--animation'");
                exit(1);
            }
            double val = result["recTime"].as< double >();
            if (val < 0) {
                std::cerr << "Reconnection time can not be negative.\n";
                exit(1);
            }
            p.reconnectionTime = val;
            p.reconnectionPics = static_cast<unsigned int>(std::ceil(p.reconnectionTime * p.framerate));
        } else if (result.count("recTime") == 0 && result.count("recPics") == 1){
            if (!p.animation){
                std::cerr << err::notAvailable("'-e' or '--recPics'", "'-a' or '--animation'");
                exit(1);
            }
            p.reconnectionPics = result["recPics"].as< unsigned int >();
            p.reconnectionTime = p.reconnectionPics / static_cast<double>(p.framerate);
        } else if (!result.count("recTime") && (!result.count("recPics"))) {}
        else {
            std::cerr << err::atMostOne("'-t', '--recTime', '-e' or '--recPics'");
            exit(1);
        }

        if (result.count("many")){
            p.many = true;
        }

        if (result.count("resolution") == 1){
            std::string res = result["resolution"].as<std::string>();
            std::string::size_type position = res.find('x');
            if (position == std::string::npos){
                position = res.find('X');
                if (position == std::string::npos){
                    std::cerr << "Option resolution has wrong format - 'x' or 'X' is missing." << "\n";
                    exit(1);
                }
            }
            std::string x = res.substr(0, position);
            std::string y = res.substr(position + 1, res.size() - position + 1);
            int dx, dy;
            try {
                dx = std::stoi(x);
                dy = std::stoi(y);
            } catch (std::invalid_argument& e){
                std::cerr << "Resolution parameter is not a number.\n";
                exit(1);
            }
            if (dx < 0 || dy < 0){
                std::cerr << "Resolution parameter can not be negative.\n";
                exit(1);
            }
            p.resolution = {dx, dy};
        } else if (result.count("resolution") > 1) {
            std::cerr << err::atMostOne("'-r' or '--resolution'");
            exit(1);
        }

        if (result.count("magnify") == 1){
            if (!p.savePicture){
                std::cerr << err::notAvailable("'-m' or '--magnify'", "'-s' or '--save'");
                exit(1);
            }
            int val = result["magnify"].as<int>();
            if (val < 0){
                std::cerr << "Magnification can not be negative.\n";
                exit(1);
            }
            p.magnify = val;
        } else if (result.count("magnify") > 1) {
            std::cerr << err::atMostOne("'-m' or '--magnify'");
            exit(1);
        }

    } catch (cxxopts::OptionException& e){
        std::cerr << e.what();
        exit(1);
    }
}

int main(int argc, char* argv[]){
    Parameters params;
    parse(argc, argv, params);

    Visualizer visualizer;
    Animator animator;
    Camera cameraStart;
    Camera cameraEnd;
    bool cameraMove;
    std::vector<Configuration> configs;
    Configuration cfg;
    std::vector<ColorRule> colorRules;

    if (params.cameraSet) {
        readCameraSettings(params.cameraSettings, cameraStart, cameraEnd, cameraMove);
    }

    if (params.colorsSet){
        readColorRules(params.colorsFile, colorRules);
    }

    if (!params.many){
        readConfiguration(params.inputFile, cfg);
        animator.visualizeOneConfig(cfg, params.path, params.savePicture, cameraStart,
                params.resolution, params.magnify, colorRules);
        return 0;
    }

    //many
    readConfigurations(params.inputFile, configs);
    if (params.animation){
        animator.visualizeMainConfigs(configs, params.phi, params.reconnectionPics, params.path,
                params.savePicture, cameraStart, cameraEnd,
                params.resolution, params.magnify, colorRules);
        return 0;
    }
    animator.visualizeAllConfigs(configs, params.path, params.savePicture, cameraStart, cameraEnd,
            params.resolution, params.magnify, colorRules);

}
