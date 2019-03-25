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
std::ifstream cameraSettings, inputFile;
bool cameraSet = false;
std::string path("../data/default");
double omega = 120;
double phi = 5;
double reconnectionTime = 2;
unsigned int reconnectionPics = 48;

void parse(int argc, char* argv[]){
    cxxopts::Options options("rofi-vis", "RoFI Visualizer: Tool for visualization of configurations and creating animations.");
    options.positional_help("[optional args]").show_positional_help();

    options.add_options()
            ("h,help", "Print help")
            ("s,save", "Save picture to file")
            ("a,animation", "Create animation from configurations")
            ("c,camera", "Camera settings file", cxxopts::value<std::string>())
            ("p,path", "Path where to save pictures", cxxopts::value<std::string>())
            ("o,omega", "Maximal angular velocity in 1°/s", cxxopts::value<double>())
            ("f,phi", "Maximal angle diff in ° per picture", cxxopts::value<double>())
            ("r,recTime", "Time in seconds for reconnection", cxxopts::value<double>())
            ("e,recPics", "Number of pictures for reconnection", cxxopts::value<unsigned int>()) //rename
            ("i,input", "Input config file", cxxopts::value<std::string>())
            ("m,many", "Many configurations in one file")
            ;

    try {
        auto result = options.parse(argc, argv);
        if (result.count("help")) {
            std::cout << options.help({"", "Group"}) << std::endl;
            exit(0);
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
            std::cerr << "There can not be more than one -c or --camera options.\n";
            exit(0);
        }

        if (result.count("path") == 1){
            path = result["path"].as< std::string >();
        } else if (result.count("path") > 1){
            std::cerr << "There can not be more than one -p or --path options.\n";
            exit(0);
        }

        if (result.count("omega") == 1 && result.count("phi") == 0){
            double val = result["omega"].as< double >();
            if (val < 0) {
                std::cerr << "Angular velocity can not be negative.\n";
                exit(0);
            }
            omega = val;
            phi = omega / 24;       //framerate 24
        } else if (result.count("omega") == 0 && result.count("phi") == 1) {
            double val = result["phi"].as< double >();
            if (val < 0) {
                std::cerr << "Maximal angle diff can not be negative.\n";
                exit(0);
            }
            phi = val;
            omega = 24 * phi;       //framerate 24
        } else if (!result.count("omega") && !result.count("phi")){}
        else {
            std::cerr << "There can not be more than one -o, --omega, -f or --phi options.\n";
            exit(0);
        }

        if (result.count("recTime") == 1 && result.count("recPics") == 0){
            double val = result["recTime"].as< double >();
            if (val < 0) {
                std::cerr << "Reconnection time can not be negative.\n";
                exit(0);
            }
            reconnectionTime = val;
            reconnectionPics = static_cast<unsigned int>(std::ceil(reconnectionTime * 24));
        } else if (result.count("recTime") == 0 && result.count("recPics") == 1){
            reconnectionPics = result["recPics"].as< unsigned int >();
            reconnectionTime = reconnectionPics / static_cast<double>(24);
        } else if (!result.count("recTime") && (!result.count("recPics"))) {}
        else {
            std::cerr << "There can not be more than one -r, --recTime, -rp or --recPics options.\n";
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
            std::cerr << "There must be exactly one -i or --input option.\n";
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