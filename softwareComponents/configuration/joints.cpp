#include "joints.hpp"

namespace rofi {

std::ostream& operator<<( std::ostream& out, Joint& j ) {
    atoms::visit( j,
        [&]( RigidJoint& ) { out << "< RigidJoint >"; },
        [&]( RotationJoint& j ) { out << "< RotationJoint: axis ( " 
                                      << j._axis[ 0 ] << ", " << j._axis[ 1 ] << ", "
                                      << j._axis[ 2 ] << ", " << j._axis[ 3 ] << " ), limits [ "
                                      << j._limits.first << ", " << j._limits.second
                                      << " ] >"; } );
    return out;
};

} // namespace rofi
