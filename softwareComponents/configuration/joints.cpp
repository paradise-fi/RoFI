#include "joints.hpp"

namespace rofi::configuration {

std::ostream& operator<<( std::ostream& out, Joint& j ) {
    atoms::visit( j,
        [&]( RigidJoint& ) { out << "< RigidJoint >"; },
        [&]( RotationJoint& j ) { out << "< RotationJoint: axis ( " 
                                      << j._axis[ 0 ] << ", " << j._axis[ 1 ] << ", "
                                      << j._axis[ 2 ] << ", " << j._axis[ 3 ] << " ), limits [ "
                                      << j._limits.first.deg() << "°, " << j._limits.second.deg()
                                      << "° ] >"; } );
    return out;
};

} // namespace rofi::configuration
