#include <iostream>

#include "controllers.hpp"
#include "gazebo_master.hpp"
#include "simulation.hpp"


int main()
{
    using namespace rofi::simplesim;

    std::cout << "Starting simplesim master" << std::endl;

    auto gzMaster = startGazeboMaster();
    auto simulation = std::make_shared< Simulation >( rofi::configuration::Rofibot() );
    auto rofiInterface = std::make_shared< RofiInterface >();

    auto rofiController = Controller::runRofiController( simulation, rofiInterface );

    std::cout << "Waiting for messages..." << std::endl;
    rofiController.wait();
}
