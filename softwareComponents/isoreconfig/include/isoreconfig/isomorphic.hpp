#include <configuration/rofiworld.hpp>

namespace rofi::isoreconfig {

/**
 * @brief "Position" (4x4 matrix)
 */
using Matrix = arma::mat44;
/**
 * @brief Container of positions
 */
using Positions = std::vector< Matrix >;


/**
 * @brief Creates a vector of positions from a given module <mod> by splitting it
 * into six distinct points, each in the center of one of the RoFIComs.
 * Assumes the module is a UniversalModule.
 * Assumes the RofiWorld the module belongs to has been prepared, so the RoFIComs 
 * have a relative position.
 * @param mod Module from which the points are calculated.
 * @return Vector of six point matrices, each in the center of a corresponding RoFICom. 
 */
Positions decomposeUniversalModule( const rofi::configuration::Module& mod );

/**
 * @brief Creates a vector of positions from a given RofiWorld <rw> by splitting 
 * every module into six distinct points, each in the center of one of the RoFIComs.
 * Assumes the RofiWorld consists only of UniversalModules.
 * Assumes the RofiWorld has been prepared, so the modules have a relative position.
 * @param rb RofiWorld from which the points are calculated.
 * @return Vector of point matrices, each in the center of a corresponding RoFICom. 
 */
std::array< Positions, 2 > decomposeRofiWorld( const rofi::configuration::RofiWorld& rw );

/**
 * @brief Calculate the center of gravity (unweighted average of module-defining
 * points) from a given RofiWorld <rw>.
 * Raises std::logic_error if the RofiWorld <rw> has not been prepared.
 * @param rb RofiWorld to calculate the center of gravity from.
 * @return Unweighted average of points defining the RofiWorld
 * (module points, not connection points).
 */
Matrix centroid( const rofi::configuration::RofiWorld& rw );

} // namespace rofi::isoreconfig
