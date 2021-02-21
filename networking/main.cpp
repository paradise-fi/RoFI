#include <iostream>

#include <gazebo/gazebo.hh>

#include "controllers.hpp"
#include "gazebo_master.hpp"
#include "simulation.hpp"


int main()
{
    using namespace rofi::networking;

    std::cout << "Starting master" << std::endl;

    auto gzMaster = startGazeboMaster();
    Simulation simulation;
    RofiInterface rofiInterface;

    runRofiController( simulation, rofiInterface );

    std::cout << "Waiting for messages..." << std::endl;
    while ( true )
    {
    }
}
