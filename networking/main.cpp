#include <iostream>

#include <gazebo/gazebo.hh>

#include "distributor.hpp"
#include "master.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>


int main()
{
    using namespace rofi::networking;

    std::cout << "Starting master" << std::endl;

    auto master = startMaster();
    Database database;
    Distributor distributor = { database };

    std::cout << "Waiting for messages..." << std::endl;
    while ( true )
    {
    }
}
