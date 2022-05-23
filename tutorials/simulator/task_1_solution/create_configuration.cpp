#include <iostream>

#include <configuration/pad.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>

/**
 * This code creates the configuration in code and prints the json serialization.
 */

rofi::configuration::RofiWorld createConfiguration()
{
    auto world = rofi::configuration::RofiWorld();

    auto um12 = world.insert( rofi::configuration::UniversalModule( 12 ) );
    auto pad42 = world.insert( rofi::configuration::Pad( 42, 6, 3 ) );

    rofi::configuration::connect<
            rofi::configuration::RigidJoint >( pad42.components().front(),
                                               rofi::configuration::matrices::Vector(),
                                               rofi::configuration::matrices::identity );
    rofi::configuration::connect( um12.getConnector( "A-Z" ),
                                  pad42.connectors()[ 1 ],
                                  rofi::configuration::roficom::Orientation::North );

    return world;
}

int main()
{
    auto world = createConfiguration();

    world.prepare();
    if ( auto result = world.isValid(); !result.first ) {
        std::cerr << "Configuration is not valid (" << result.second << ")\n";
        return 1;
    }

    std::cout << rofi::configuration::serialization::toJSON( world ) << "\n";
}
