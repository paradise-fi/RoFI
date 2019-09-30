#include <gazebo/gazebo_config.h>
#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>

#include <jointVelCmd.pb.h>

#include <iostream>

int main(int argc, char **argv) {
    try {
        gazebo::client::setup(argc, argv);

        gazebo::transport::NodePtr node(new gazebo::transport::Node());
        node->Init();

        gazebo::transport::PublisherPtr pub =
            node->Advertise<rofi::messages::JointVelCmd>("~/universalModule/control");

        std::cerr << "Waiting for connection...\n";
        pub->WaitForConnection();
        std::cerr << "Connected\n";

        rofi::messages::JointVelCmd cmd;
        cmd.set_joint(0);
        cmd.set_velocity(std::atof(argv[1]));

        pub->Publish(cmd);
        gazebo::client::shutdown();
    }
    catch( const gazebo::common::Exception& e ) {
        std::cerr << e.GetErrorStr() << "\n";
    }
}