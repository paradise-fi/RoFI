#include "joints.h"

namespace rofi {

std::ostream& operator<<( std::ostream& out, Joint& j ) {
    // ToDo: Make the text representation more informative
    atoms::visit( j,
        [&]( RigidJoint& ) { out << "< RigidJoint >"; },
        [&]( RotationJoint& ) { out << "< RotationJoint >"; });
    return out;
};

} // namespace rofi