#include "rofibot.hpp"

namespace rofi::configuration {

double roficom::orientationToAngle( roficom::Orientation o ) {
    using namespace roficom;
    switch ( o ) {
        case Orientation::North:
            return M_PI;
        case Orientation::East:
            return - M_PI_2;
        case Orientation::South:
            return 0;
        case Orientation::West:
            return M_PI_2;
    }
    assert( false && "Orientation was modified" );
    return 0; // never reached
}

void Module::setJointParams( int idx, const Joint::Position& p ) {
    // Currently we invalidate all positions; ToDo: think if we can improve it
    assert( idx < _joints.size() && idx >= 0 );
    assert( _joints[ idx ].joint->position );
    _joints[ idx ].joint->position = p;
    _componentPosition = std::nullopt;
    if ( parent )
        parent->onModuleMove( id );
}

void connect( const Component& c1, const Component& c2, roficom::Orientation o ) {
    if ( c1.parent->parent != c2.parent->parent )
        throw std::logic_error( "Components have to be in the same rofibot" );
    Rofibot& bot = *c1.parent->parent;
    Rofibot::ModuleInfo& m1info = bot._modules[ c1.parent->id ];
    Rofibot::ModuleInfo& m2info = bot._modules[ c2.parent->id ];

    auto jointId = bot._moduleJoints.insert( {
        o, m1info.module->id, m2info.module->id,
        m1info.module->componentIdx( c1 ), m2info.module->componentIdx( c2 )
    } );

    m1info.outJointsIdx.push_back( jointId );
    m2info.inJointsIdx.push_back( jointId );

    bot._prepared = false;
}

} // namespace rofi::configuration
