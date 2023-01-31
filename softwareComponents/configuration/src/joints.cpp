#include <configuration/joints.hpp>

namespace rofi::configuration {

std::ostream& operator<<( std::ostream& out, Joint& joint ) {
    atoms::visit( joint,
        [ &out ]( RigidJoint& ) { out << "< RigidJoint >"; },
        [ &out ]( RotationJoint& j ) { out << "< RotationJoint: axis ( "
                                      << j._axis[ 0 ] << ", " << j._axis[ 1 ] << ", "
                                      << j._axis[ 2 ] << ", " << j._axis[ 3 ] << " ), "
                                      << "parameter " << j.position().deg() << "°, limits [ "
                                      << j.jointLimit().first.deg() << "°, " << j.jointLimit().second.deg()
                                      << "° ] >"; },
        [ &out ]( ModularRotationJoint& j ) { out << "< ModularRotationJoint: axis ( "
                                      << j._axis[ 0 ] << ", " << j._axis[ 1 ] << ", "
                                      << j._axis[ 2 ] << ", " << j._axis[ 3 ] << " ), "
                                      << "parameter " << j.position().deg() << "°, "
                                      << "modulo " << j.moduloLimit().deg() << "° >"; });
    return out;
}

} // namespace rofi::configuration
