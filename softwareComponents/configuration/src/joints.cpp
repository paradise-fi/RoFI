#include <configuration/joints.hpp>

namespace rofi::configuration {

std::ostream& operator<<( std::ostream& out, Joint& j ) {
    atoms::visit( j,
        [ &out ]( RigidJoint& ) { out << "< RigidJoint >"; },
        [ &out ]( RotationJoint& j ) { out << "< RotationJoint: axis ( "
                                      << j._axis[ 0 ] << ", " << j._axis[ 1 ] << ", "
                                      << j._axis[ 2 ] << ", " << j._axis[ 3 ] << " ), limits [ "
                                      << j.jointLimit().first.deg() << "°, " << j.jointLimit().second.deg()
                                      << "° ] >"; } );
    return out;
};

} // namespace rofi::configuration
