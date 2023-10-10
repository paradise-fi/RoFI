#include <configuration/rofiworld.hpp>
#include <isoreconfig/geometry.hpp>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

/**
 * @brief Position matrix
 */
using Matrix = arma::mat44;

/**
 * @brief Extends Vector to a matrix.
 */
Matrix pointMatrix( const Vector& pt );

/**
 * @brief Decomposes given module <mod> into distinct points, 
 * each in the center of one of the RoFIComs.
 * Assumes the RofiWorld the module belongs to has been prepared, 
 * so the RoFIComs have a relative position in space.
 * @param mod Module from which the points are calculated.
 * @return Container of points, each in the center of a corresponding RoFICom.
 */
std::vector< Vector > decomposeModule( const Module& mod );

/**
 * @brief Decomposes given RofiWorld <rw> into two containers of points;
 * first container contains points aquired by decomposing each module
 * (a point in the center of every RoFICom),
 * second container contains points in the centres of connected RoFIComs.
 * Assumes the RofiWorld has been prepared, so the modules have a relative position.
 * @param rw RofiWorld from which the points are calculated.
 * @return First container contains RoFICom points, second contains connection points.
 */
std::tuple< std::vector< Vector >, std::vector< Vector > > 
    decomposeRofiWorld( const RofiWorld& rw );

std::vector< std::vector< Vector > > decomposeRofiWorldModules( const RofiWorld& rw );

/**
 * @brief Converts a RofiWorld to a Cloud of Points.
 */
Cloud rofiWorldToCloud( const RofiWorld& rw );

/**
 * @brief Converts a RofiWorld to a canonical cloud of points,
 * which uniquely defines its shape.
 */
Cloud rofiWorldToShape( const RofiWorld& rw );

std::array< int, 4 > rofiWorldToEigenValues( const RofiWorld& rw );

/**
 * @brief Calculate the centroid from a given RofiWorld <rw>.
 * Raises std::logic_error if the RofiWorld <rw> has not been prepared.
 * @param rw RofiWorld to calculate the centroid from.
 * @return Unweighted average of points defining the RofiWorld.
 */
Vector centroid( const RofiWorld& rw );

/**
 * @brief Decides if given rofiworlds have the same physical shape.
 * Decomposes the worlds into points, applies PCA transformation 
 * and attempts to find an orthogonal transformation
 * which transforms one set of points into the other.
 */
bool equalShape( const RofiWorld& rw1, const RofiWorld& rw2 );

} // namespace rofi::isoreconfig
