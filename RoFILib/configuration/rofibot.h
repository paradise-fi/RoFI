#pragma once

#include <armadillo>

namespace rofi {

using Vector = arma::vec4;
using Matrix = arma::mat44;
using ModuleId = int;

namespace detail {

class JointBase {
public:
    virtual ~Joint() = default;
    virtual int getJointCount() const = 0;
    virtual Matrix sourceToDest() const = 0;
    virtual Matrix destToSource() const = 0;
    virtual std::pair< double, double > getJointLimits( int jointIdx ) const = 0;

    std::vector< double > positions;
};

} // namespace detail

enum class ComponentType {
    UmShoe, UmBody,
    Roficom
};

enum class ModuleType {
    Unknown,
    Universal,
    Cube,
};

class VisitorBase;

class Visitable {
    virtual void accept( VisitorBase& ) = 0;
};

class JointVisitor() {
public:
    virtual void operator()( RotationalJoint& ) = 0;
    virtual void operator()( RigidJoint& ) = 0;
};

template < typename Self >
class Joint: public detail::JointBase, public Visitable {
    void accept( VisitorBase &v ) override {
        Self* upcast = static_cast< Self *>( this );
        v( *upcast );
    }
};

class RotationalJoint: public Joint< RotationalJoint > {
public:
    RotationalJoint( Vector translation, Vector axis, Vector axisPosition ) {}

    int getJointCount() const override {
        return 1;
    }

    Matrix sourceToDest() const override {
        // ToDo
    }

    Matrix destToSource() const override {
        // ToDo
    }
};

class RigidJoint: public Joint< RigidJoint > {
public:
    int getJointCount() const override {
        return 0;
    }

    Matrix sourceToDest() const override {
        // ToDo
    }

    Matrix destToSource() const override {
        // ToDo
    }
};

class IntraModuleJoint {
public:
    std::unique_ptr< Joint > joint;
    const int sourceComponent, destinationComponent;
}

class Component {
public:
    ComponentType type;
    Module *parent;

    const std::vector< int > joints; // ?
};

class Rofibot;

class ModuleModuleJoint {
    std::unique_ptr< Joint > joint;
    ModuleId sourceModule;
    ModuleId destModule;
    int sourceComponent;
    int destComponent;
};

class SpaceModuleJoint {
    std::unique_ptr< Joint > joint;
    /*const*/ Matrix refPosition;
    const ModuleId destModule;
    const int destComponent;
};

class Module {
public:
    ModuleId id;
    ModuleType type;
    std::vector< Component > components;
    std::vector< std::unique_ptr< Joint > > joints;
    Rofibot* parent;
};

class Rofibot {
public:
    std::vector< Module > _modules;
    std::vector< unique_ptr< ModuleModuleJoint > > _moduleJoints;
    std::vector< unique_ptr< SpaceModuleJoint > > _spaceJoints;

    std::unordered_map< ModuleId, std::vector< ModuleModuleJoint* > > _jointMap;
};

class UniversalModule: public Module {

};

Module buildUniversalModule( double alpha, double beta, double gamma ) {
    Module m;
    m.components = {
        Component( &m, ComponentType::UmShoe ),
        Component( &m, ComponentType::UmBody ),
        Component( &m, ComponentType::UmBody ),
        Component( &m, ComponentType::UmShoe ),
        Component( &m, ComponentType::Roficom ),
        Component( &m, ComponentType::Roficom ),
        Component( &m, ComponentType::Roficom ),
        Component( &m, ComponentType::Roficom ),
        Component( &m, ComponentType::Roficom ),
        Component( &m, ComponentType::Roficom )
    };
    m.joints = {
        IntraModuleJoint(
            make_unique< RotationalJoint >( t1, ax1, axOr1, -90, 90 ),
            0, 1 ),
        IntraModuleJoint(
            make_unique< RotationalJoint >( t2, ax2, axOr2, -360, 360 ),
            1, 2),
        IntraModuleJoint(
            make_unique< RotationalJoint >( t3, ax3, axOr3, -90, 90 ),
            2, 3),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        ),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        ),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        ),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        ),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        ),
        IntraModuleJoint(
            make_unique< ShoeConJoint >( XMinus ),
            0, 4
        )
    }
    m.joints[ 0 ].positions = { alpha };
    m.joints[ 1 ].positions = { beta };
    m.joints[ 2 ].positions = { gamma };
    return m;
}



} // namespace rofi