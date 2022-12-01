#include <configuration/rofiworld.hpp>


/**
 * @brief Parses rofi world from given \p inputFile .
 * If file has .json extension, assumes the JSON format of rofi world;
 * otherwise assumes the world is in the old (viki) format.
 * Returns an error if the format is incorrect.
 * @param inputFile path to file containing the rofi world
 * @returns the parsed rofi world
 */
atoms::Result< rofi::configuration::RofiWorld > parseRofiWorld( const std::string & inputFile );

/**
 * @brief Affixes given \p world in space using a `RigidJoint` between
 * (0, 0, 0) and first body of first module.
 * Assumes the world is not empty.
 * @param world rofi world to be affixed in space
 */
void affixRofiWorld( rofi::configuration::RofiWorld & world );
