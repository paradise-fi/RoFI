#include <iostream>
#include <fstream>
#include <string_view>
#include "../Reader.h"
#include "Visualizer.h"
#include "AnimationReader.h"
#include "Animator.h"
#include "../cxxopts.hpp"

bool many = false;
bool savePicture = false;
bool animation = false;
std::ifstream inputFile;
std::ifstream cameraSettings;
bool cameraSet = false;
std::string path("../data/default");
double omega = 120;
double phi = 5;
double reconnectionTime = 2;
unsigned int reconnectionPics = 48;
unsigned int framerate = 24;

std::string notAvailable(const std::string& option, const std::string& condition){
    std::stringstream str;
    str << "The option " << option << " is available only with the option " << condition << ".\n";
    return str.str();
}

std::string atMostOne(const std::string& option){
    std::stringstream str;
    str << "There can not be at most one " << option << "option.\n";
    return str.str();
}

std::string exactlyOne(const std::string& option){
    std::stringstream str;
    str << "There must be exactly one " << option << " option\n";
    return str.str();
}

void parse(int argc, char* argv[]){
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
            ("o,omega", "Maximal angular velocity in 1°/s", cxxopts::value<double>())
            ("d,degree", "Maximal angle diff in ° per picture", cxxopts::value<double>())
            ("r,recTime", "Time in seconds for reconnection", cxxopts::value<double>())
            ("e,recPics", "Number of pictures for reconnection", cxxopts::value<unsigned int>()) //rename
            ("m,many", "Many configurations in one file")
            ;

    try {
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
        }

        if (result.count("input") == 1){
            std::string filename(result["input"].as< std::string >());
            inputFile.open(filename);
            if (!inputFile.good()){
                std::cerr << "Could not open file " << filename << ".\n";
                exit(0);
            }
        } else {
            std::cerr << exactlyOne("'-i' or '--input'");
        }

        if (result.count("save")){
            savePicture = true;
        }

        if (result.count("animation")){
            animation = true;
            many = true;
        }

        if (result.count("camera") == 1){
            std::string filename = result["camera"].as< std::string >();
            cameraSettings.open(filename);
            if (!cameraSettings.good()) {
                std::cerr << "Could not open file " << filename << ".\n";
                exit(0);
            }
            cameraSet = true;
        } else if (result.count("camera") > 1) {
            std::cerr << atMostOne("'-c' or '--camera'");
            exit(0);
        }

        if (result.count("path") == 1){
            path = result["path"].as< std::string >();
            if (!result.count("save")){
                std::cerr << "Path is not used because -s or --save is not set.\n";
            }
        } else if (result.count("path") > 1){
            std::cerr << atMostOne("'-p' or '--path'");
            exit(0);
        }

        if (result.count("framerate") == 1){
            if (!animation){
                std::cerr << notAvailable("'-f' or '--framerate'", "'-a' or '--animation'");
                exit(0);
            }
            framerate = result["framerate"].as<unsigned int>();
        } else if (result.count("framerate") > 1){
            std::cerr << atMostOne("'-f' or '--framerate'");
            exit(0);
        }

        if (result.count("omega") == 1 && result.count("degree") == 0){
            if (!animation){
                std::cerr << notAvailable("'-o' or '--omega'", "'-a' or '--animation'");
                exit(0);
            }
            double val = result["omega"].as< double >();
            if (val < 0) {
                std::cerr << "Angular velocity can not be negative.\n";
                exit(0);
            }
            omega = val;
            phi = omega / framerate;
        } else if (result.count("omega") == 0 && result.count("degree") == 1) {
            if (!animation){
                std::cerr << notAvailable("'-d' or '--degree'", "'-a' or '--animation'");
                exit(0);
            }
            double val = result["degree"].as< double >();
            if (val < 0) {
                std::cerr << "Maximal angle diff can not be negative.\n";
                exit(0);
            }
            phi = val;
            omega = framerate * phi;
        } else if (!result.count("omega") && !result.count("degree")){}
        else {
            std::cerr << atMostOne("'-o', '--omega', '-d' or '--degree'");
            exit(0);
        }

        if (result.count("recTime") == 1 && result.count("recPics") == 0){
            if (!animation){
                std::cerr << notAvailable("'-r' or '--recTime'", "'-a' or '--animation'");
                exit(0);
            }
            double val = result["recTime"].as< double >();
            if (val < 0) {
                std::cerr << "Reconnection time can not be negative.\n";
                exit(0);
            }
            reconnectionTime = val;
            reconnectionPics = static_cast<unsigned int>(std::ceil(reconnectionTime * framerate));
        } else if (result.count("recTime") == 0 && result.count("recPics") == 1){
            if (!animation){
                std::cerr << notAvailable("'-e' or '--recPics'", "'-a' or '--animation'");
                exit(0);
            }
            reconnectionPics = result["recPics"].as< unsigned int >();
            reconnectionTime = reconnectionPics / static_cast<double>(framerate);
        } else if (!result.count("recTime") && (!result.count("recPics"))) {}
        else {
            std::cerr << atMostOne("'-r', '--recTime', '-e' or '--recPics'");
            exit(0);
        }

        if (result.count("many")){
            many = true;
        }

    } catch (cxxopts::OptionException& e){
        std::cerr << e.what();
        exit(0);
    }
}

unsigned long Animator::outCount = 0;

int main(int argc, char* argv[]){
    parse(argc, argv);

    if (savePicture){
        std::ofstream videoParams("../visualizer/.videoParams.txt");
        if (!videoParams.good()){
            std::cerr << "Could not write parameters for video to file ../visualizer/.videoParams.txt\n";
        }
        videoParams << path << "\n";
        videoParams << framerate << "\n";
    }

    Reader reader;
    Visualizer visualizer;
    Animator animator;
    Camera cameraStart;
    Camera cameraEnd;
    bool cameraMove;
    std::vector<Configuration> configs;
    Configuration cfg;

    if (cameraSet) {
        reader.readCameraSettings(cameraSettings, cameraStart, cameraEnd, cameraMove);
    }

    if (!many){
        reader.read(inputFile, cfg);
        animator.visualizeOneConfig(cfg, path, savePicture, cameraStart);
        return 0;
    }

    //many
    reader.read(inputFile, configs);
    if (animation){
        animator.visualizeMainConfigs(configs, phi, reconnectionPics, path, savePicture, cameraStart, cameraEnd);
        return 0;
    }
    animator.visualizeAllConfigs(configs, path, savePicture, cameraStart, cameraEnd);

}