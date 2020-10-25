#include <iostream>

#include <gazebo/gazebo.hh>

#include "master.hpp"

#include "distributorReq.pb.h"
#include "distributorResp.pb.h"


void onDistributorRequest( const boost::shared_ptr< const rofi::messages::DistributorReq > & req )
{
    std::cout << "Got request:\n" << req->DebugString() << std::endl;
}

int main( int argc, char ** argv )
{
    std::cout << "Starting master" << std::endl;

    startMaster( argc, argv );

    auto node = boost::make_shared< gazebo::transport::Node >();
    node->Init();

    auto sub = node->Subscribe( "~/distributor/request", &onDistributorRequest );


    std::cout << "Waiting for messages..." << std::endl;
    while ( true )
    {
    }
}
