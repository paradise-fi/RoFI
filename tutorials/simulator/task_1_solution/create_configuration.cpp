#include <iostream>

#include <configuration/pad.hpp>
#include <configuration/rofibot.hpp>
#include <configuration/serialization.hpp>

/**
 * This code creates the configuration in code and prints the json serialization.
 */

rofi::configuration::Rofibot createConfiguration()
{
    auto rofibot = rofi::configuration::Rofibot();

    auto um12 = rofibot.insert( rofi::configuration::UniversalModule( 12 ) );
    auto pad42 = rofibot.insert( rofi::configuration::Pad( 42, 6, 3 ) );

    rofi::configuration::connect<
            rofi::configuration::RigidJoint >( pad42.components().front(),
                                               rofi::configuration::matrices::Vector(),
                                               rofi::configuration::matrices::identity );
    rofi::configuration::connect( um12.getConnector( "A-Z" ),
                                  pad42.connectors()[ 1 ],
                                  rofi::configuration::roficom::Orientation::North );

    return rofibot;
}

int main()
{
    auto rofibot = createConfiguration();

    rofibot.prepare();
    if ( auto result = rofibot.isValid(); !result.first ) {
        std::cerr << "Configuration is not valid (" << result.second << ")\n";
        return 1;
    }

    std::cout << rofi::configuration::serialization::toJSON( rofibot ) << "\n";
}
