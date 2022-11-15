#include <dimcli/cli.h>
#include <configuration/rofiworld.hpp>

int build( Dim::Cli & /* cli */ );
int check( Dim::Cli & /* cli */ );
int points( Dim::Cli & /* cli */ );
int preview( Dim::Cli & /* cli */ );

/**
 * @brief Parses configuration from given <inputFile>.
 * If file has .json extension, assumes the JSON format of configuration;
 * otherwise assumes the configuration is in the old (viki) format.
 * Throws an error if the format is not recognized.
 * @param inputFile File containing the configuration.
 * @return Returns the parsed configuration.
 */
rofi::configuration::RofiWorld parseConfiguration( const std::string& inputFile );

/**
 * @brief Affixes given <configuration> in space using a RigidJoint between
 * (0, 0, 0) and first found module.
 * Assumes the configuration is not empty.
 * @param configuration Configuration to be affixed in space.
 */
void affixConfiguration( rofi::configuration::RofiWorld& configuration );
