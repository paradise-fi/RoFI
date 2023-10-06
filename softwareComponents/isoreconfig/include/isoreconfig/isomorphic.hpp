#include <configuration/rofiworld.hpp>
#include <isoreconfig/geometry.hpp>

namespace rofi::isoreconfig {

/**
 * @brief Position matrix
 */
using Matrix = arma::mat44;

/**
 * @brief Extends a point to a matrix.
 */
Matrix pointMatrix( const Vector& pt );

/**
 * @brief Decomposes given module <mod> into distinct points, 
 * each in the center of one of the RoFIComs.
 * Assumes the RofiWorld the module belongs to has been prepared, 
 * so the RoFIComs have a relative position in space.
 * @param rModule Module from which the points are calculated.
 * @return Container of points, each in the center of a corresponding RoFICom.
 */
std::vector< Vector > decomposeModule( const rofi::configuration::Module& rModule );

/**
 * @brief Decomposes given RofiWorld <rw> into two containers of points;
 * first container contains points aquired by decomposing each module
 * (a point in the center of every RoFICom),
 * second container contains points in the centres of connected RoFIComs.
 * Assumes the RofiWorld has been prepared, so the modules have a relative position.
 * @param rw RofiWorld from which the points are calculated.
 * @return First container contains RoFICom points, second contains connection points.
 */
std::tuple< std::vector< Vector >, std::vector< Vector > > decomposeRofiWorld( 
    const rofi::configuration::RofiWorld& rw );

/**
 * @brief Converts a RofiWorld to a Cloud of Points.
 */
Cloud rofiWorldToCloud( const rofi::configuration::RofiWorld& rw );

/**
 * @brief Calculate the centroid from a given RofiWorld <rw>.
 * Raises std::logic_error if the RofiWorld <rw> has not been prepared.
 * @param rw RofiWorld to calculate the centroid from.
 * @return Unweighted average of points defining the RofiWorld.
 */
Vector centroid( const rofi::configuration::RofiWorld& rw );

/**
 * @brief Calculate the centroid (unweighted average) of given points.
 * Assumes the container is not empty.
 * @param pts Points to calculate the center of gravity from.
 * @return Centroid of given points.
 */
Vector centroid( const std::vector< Vector >& pts );

/**
 * @brief Decides if given rofiworlds have the same physical shape.
 * Decomposes the worlds into points, applies PCA transformation 
 * and attempts to find an orthogonal transformation
 * which transforms one set of points into the other.
 */
bool equalShape( 
    const rofi::configuration::RofiWorld& rw1, 
    const rofi::configuration::RofiWorld& rw2 );

} // namespace rofi::isoreconfig
