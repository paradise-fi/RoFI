#include <iostream>
#include <fstream>
#include <string_view>
#include "../Reader.h"
#include "Visualizer.h"
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
std::pair<unsigned long, unsigned long> resolution = {1920, 1080};
int magnify = 1;

std::string notAvailable(const std::string& option, const std::string& condition){
    std::stringstream str;
    str << "The option " << option << " is available only with the option " << condition << ".\n";
    return str.str();
}

std::string atMostOne(const std::string& option){
    std::stringstream str;
    str << "There can not be at most one " << option << " option.\n";
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
            ("v,velocity", "Maximal angular velocity in 1°/s", cxxopts::value<double>())
            ("g,angle", "Maximal angle diff in ° per picture", cxxopts::value<double>())
            ("t,recTime", "Time in seconds for reconnection", cxxopts::value<double>())
            ("e,recPics", "Number of pictures for reconnection", cxxopts::value<unsigned int>()) //rename
            ("n,many", "Many configurations in one file")
            ("r,resolution", "Size of the window on the screen or resolution of the saved picture in format numberxnumber", cxxopts::value<std::string>())
            ("m,magnify", "Magnification of saved pictures", cxxopts::value<int>())
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
                std::cerr << "Path is not used because '-s' or '--save' is not set.\n";
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

        if (result.count("velocity") == 1 && result.count("angle") == 0){
            if (!animation){
                std::cerr << notAvailable("'-v' or '--velocity'", "'-a' or '--animation'");
                exit(0);
            }
            double val = result["velocity"].as< double >();
            if (val < 0) {
                std::cerr << "Angular velocity can not be negative.\n";
                exit(0);
            }
            omega = val;
            phi = omega / framerate;
        } else if (result.count("velocity") == 0 && result.count("angle") == 1) {
            if (!animation){
                std::cerr << notAvailable("'-g' or '--angle'", "'-a' or '--animation'");
                exit(0);
            }
            double val = result["angle"].as< double >();
            if (val < 0) {
                std::cerr << "Maximal angle diff can not be negative.\n";
                exit(0);
            }
            phi = val;
            omega = framerate * phi;
        } else if (!result.count("velocity") && !result.count("angle")){}
        else {
            std::cerr << atMostOne("'-v', '--velocity', '-g' or '--angle'");
            exit(0);
        }

        if (result.count("recTime") == 1 && result.count("recPics") == 0){
            if (!animation){
                std::cerr << notAvailable("'-t' or '--recTime'", "'-a' or '--animation'");
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
            std::cerr << atMostOne("'-t', '--recTime', '-e' or '--recPics'");
            exit(0);
        }

        if (result.count("many")){
            many = true;
        }

        if (result.count("resolution") == 1){
            std::string res = result["resolution"].as<std::string>();
            std::string::size_type position = res.find('x');
            if (position == std::string::npos){
                position = res.find('X');
                if (position == std::string::npos){
                    std::cerr << "Option resolution has wrong format - 'x' or 'X' is missing" << "\n";
                    exit(0);
                }
            }
            std::string x = res.substr(0, position);
            std::string y = res.substr(position + 1, res.size() - position + 1);
            unsigned long dx, dy;
            try {
                dx = std::stoul(x);
                dy = std::stoul(y);
            } catch (std::invalid_argument& e){
                std::cerr << "Resolution parameter is not a number.";
                exit(0);
            }
            resolution = {dx, dy};
        } else if (result.count("resolution") > 1) {
            std::cerr << atMostOne("'-r' or '--resolution'");
            exit(0);
        }

        if (result.count("magnify") == 1){
            if (!savePicture){
                std::cerr << notAvailable("'-m' or '--magnify'", "'-s' or '--save'");
                exit(0);
            }
            magnify = result["magnify"].as<int>();
        } else if (result.count("magnify") > 1) {
            std::cerr << atMostOne("'-m' or '--magnify'");
            exit(0);
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
        animator.visualizeOneConfig(cfg, path, savePicture, cameraStart, resolution, magnify);
        return 0;
    }

    //many
    reader.read(inputFile, configs);
    if (animation){
        animator.visualizeMainConfigs(configs, phi, reconnectionPics, path, savePicture, cameraStart, cameraEnd,
                resolution, magnify);
        return 0;
    }
    animator.visualizeAllConfigs(configs, path, savePicture, cameraStart, cameraEnd, resolution, magnify);

}