#pragma once

#include <memory>
#include <vector>
#include <set>
#include <map>
#include <ranges>

#include <atoms/containers.hpp>
#include <atoms/algorithm.hpp>
#include <atoms/units.hpp>
#include <tcb/span.hpp>
#include <fmt/format.h>

#include <configuration/joints.hpp>

namespace rofi::configuration {

/// ModuleId
using ModuleId = int;

enum class ComponentType {
    UmShoe, UmBody,
    Roficom
};

enum class ModuleType {
    Unknown,
    Universal,
    Cube,
    Pad,
};

namespace roficom {
    enum class Orientation { North, East, South, West };

    /**
     * Return a corresponding angle in radians for a given orientation
     */
    double orientationToAngle( Orientation o = Orientation::North );
} // namespace roficom

/**
 * \brief Joint between two components of the same module
 */
struct ComponentJoint {
    ComponentJoint( std::unique_ptr< Joint > joint, int source, int dest ):
        joint( std::move( joint ) ),
        sourceComponent( source ),
        destinationComponent( dest )
    {}

    atoms::ValuePtr< Joint > joint;
    int sourceComponent, destinationComponent;
};

template< typename JointT, typename...Args >
ComponentJoint makeComponentJoint( int source, int dest, Args&&...args ) {
    return ComponentJoint{
        std::make_unique< JointT >( ( std::forward< Args >( args ) )... ),
        source, dest };
}

/**
 * \brief Joint between two modules.
 *
 * The joint is created between two components of two modules specified by
 * module ids and corresponding component index.
 */
struct RoficomJoint: public Joint {
    RoficomJoint( roficom::Orientation o, ModuleId sourceModule, ModuleId destModule,
        int sourceConnector, int destConnector)
    : orientation( o ), sourceModule( sourceModule ), destModule( destModule ),
      sourceConnector( sourceConnector ), destConnector( destConnector )
    {}

    int paramCount() const override {
        return 0;
    }

    Matrix sourceToDest() const override {
        assert( positions.empty() );
        // the "default" roficom is A-X
        return translate( { -1, 0, 0 } ) * rotate( M_PI, { 0, 0, 1 } )
            * rotate( M_PI, { 1, 0, 0 } )
            * rotate( roficom::orientationToAngle( orientation ), { -1, 0, 0 } );
    }

    std::pair< Angle, Angle > jointLimits( int ) const override {
        throw std::logic_error( "RoFICoM joint has no parameters" );
    }

    ATOMS_CLONEABLE( RoficomJoint );

    roficom::Orientation orientation;
    ModuleId sourceModule;
    ModuleId destModule;
    int sourceConnector;
    int destConnector;
};

/**
 * \brief Joint between a fixed point in space and a module
 */
struct SpaceJoint {
    atoms::ValuePtr< Joint > joint;
    Vector refPoint;
    ModuleId destModule;
    int destComponent;
};

class Rofibot;
class Module;

/**
 * \brief Single body of a module
 */
struct Component {
    using JointId = int;

    ComponentType type;
    std::vector< JointId > inJoints;
    std::vector< JointId > outJoints;

    Module *parent;

    bool operator==( const Component& o ) const {
        return type == o.type &&
               inJoints == o.inJoints &&
               outJoints == o.outJoints &&
               parent == o.parent;
    }

    bool operator!=( const Component& o ) const {
        return !( *this == o );
    }
};

/**
 * \brief RoFI module
 *
 * The module is composed out of components.
 */
class Module {
public:
    /**
     * \brief Construct module
     *
     * Note that components contain all module components out of which first \p
     * connectorCount are module connectors.
     *
     * The user can optionally specify a root component (e.g, in case of cyclic
     * joints). If no root component is specified, component with no ingoing
     * joints is selected.
     */
    Module( ModuleType type,
        std::vector< Component > components,
        int connectorCount,
        std::vector< ComponentJoint > joints,
        ModuleId id,
        std::optional< int > rootComponent = std::nullopt )
    : type( type ), _id( id ),
      _components( std::move( components ) ),
      _connectorCount( connectorCount ),
      _joints( std::move( joints ) ),
      parent( nullptr ),
      _rootComponent( rootComponent )
    {
        assert( _components.size() > 0 && "Module has to have at least one component" );
        _prepareComponents();
        if ( !_rootComponent )
            _rootComponent = _computeRoot();
    }

    ATOMS_CLONEABLE_BASE(Module);

    virtual ~Module() = default;

    ModuleId getId() const {
        return _id;
    }

    /** \brief Set ID of the module to the new value
     * 
     * Checks if the new value is not used within its parental rofibot,
     * if it's in use, the ID is left the same and the method returns
     * False. Otherwise the ID is changed. Returns True on success.
     */
    bool setId( ModuleId newId );

    void setJointParams( int idx, const Joint::Positions& p );
    // Implemented in CPP files as it depends on definition of Rofibot

    /**
     * \brief Get a component position relative to module origin
     *
     * Raises std::logic_error if the components are inconsistent
     */
    Matrix getComponentPosition( int idx ) {
        assert( idx < _components.size() );
        if ( !_componentPosition )
            prepare();
        return _componentPosition.value()[ idx ];
    }

    /**
     * \brief Get a component position within coordinate system specified by the
     * second argument.
     *
     * Raises std::logic_error if the components are not prepared
     */
    Matrix getComponentPosition( int idx, Matrix position ) const {
        assert( idx >= 0 && idx < _components.size() );
        if ( !_componentPosition )
            throw std::logic_error( "Module is not prepared" );
        return position * _componentPosition.value()[ idx ];
    }

    /**
     * \brief Get a vector of occupied positions relative to module origin
     *
     * Raises std::logic_error if the components are not prepared
     */
    std::vector< Matrix > getOccupiedPositions() const {
        if ( !_componentPosition )
            throw std::logic_error( "Module is not prepared" );

        std::vector< Matrix > res;
        for ( auto& m : _componentPosition.value() ) {
            res.push_back( translate( center( m ) ) );
        }
        std::sort( res.begin(), res.end(), []( auto a, auto b ) { return equals( a, b ); } );
        res.erase( std::unique( res.begin(), res.end(), []( auto a, auto b ) { return equals( a, b ); } ), res.end() );

        return res;
    }

    /**
     * \brief Precompute component positions
     *
     * Raises std::logic_error if the components are inconsistent
     */
    void prepare() {
        std::vector< Matrix > positions( _components.size() );
        std::vector< bool > initialized( _components.size() );

        auto dfsTraverse = [&]( int compIdx, Matrix position, auto& self ) {
            if ( initialized[ compIdx ] ) {
                if ( !equals( position, positions[ compIdx ] ) )
                    throw std::logic_error( "Inconsistent component positions" );
                return;
            }
            positions[ compIdx ] = position;
            initialized[ compIdx ] = true;
            for ( int outJointIdx : _components[ compIdx ].outJoints ) {
                const ComponentJoint& j = _joints[ outJointIdx ];
                self( j.destinationComponent, position * j.joint->sourceToDest(), self );
            }
        };
        dfsTraverse( _rootComponent.value(), identity, dfsTraverse );

        if ( !atoms::all( initialized ) )
            throw std::logic_error( "There are components without position" );
        _componentPosition = std::move( positions );
    }

    auto configurableJoints() {
        return _joints | std::views::transform( []( ComponentJoint& cj ) { return cj.joint; } )
                       | std::views::filter( []( const atoms::ValuePtr< Joint >& ptr ) {
                                                    return ptr->paramCount() > 0;
                                                } );
    }

    /**
     * \brief Get read-only view of the components
     */
    tcb::span< const Component > components() const {
        return _components;
    }

    /**
     * \brief Get read-only view of the bodies
     */
    tcb::span< const Component > bodies() const {
        return components().subspan( _connectorCount );
    }

    tcb::span< const ComponentJoint > joints() const {
        return _joints;
    }

    /**
     * \brief Get read-only view of the connectors
     */
    tcb::span< const Component > connectors() {
        return components().subspan( 0, _connectorCount );
    };

    /**
     * \brief Get index of a component
     */
    int componentIdx( const Component& c ) {
        int idx = 0;
        for ( const Component& x : _components ) {
            if ( x == c )
                return idx;
            idx++;
        }
        throw std::logic_error( "Component does not belong to the module" );
    }

    ModuleType type; ///< module type
    Rofibot* parent; ///< pointer to parenting Rofibot
private:
    ModuleId _id = 0; ///< integral identifier unique within a context of a single rofibot
    std::vector< Component > _components; ///< All module components, first _connectorCount are connectors
    int _connectorCount;
    std::vector< ComponentJoint > _joints;
    std::optional< int > _rootComponent;

    std::optional< std::vector< Matrix > > _componentPosition;

    /**
     * \brief computes back references to joints in components
     */
    void _prepareComponents() {
        for ( auto& c: _components ) {
            c.outJoints.clear();
            c.inJoints.clear();
            c.parent = this;
        }
        for ( Component::JointId i = 0; i != _joints.size(); i++ ) {
            const auto& j = _joints[ i ];
            _components[ j.sourceComponent ].outJoints.push_back( i );
            _components[ j.destinationComponent ].inJoints.push_back( i );
        }
        for ( auto& c: _components )
            c.parent = this;
    }

    /**
     * \brief Get index of root component - component with no ingoing edges
     */
    int _computeRoot() const {
        for ( int i = 0; i != _components.size(); i++ )
            if ( _components[ i ].inJoints.size() == 0 )
                return i;
        throw std::logic_error( "Module does not have a root component" );
    }

    friend class Rofibot;
};

/**
 * \brief Collision model ignoring collision
 */
class NoColision {
public:
    /**
     * \brief Decide if two modules collide
     */
    bool operator()( const Module& a, const Module& b, Matrix posA, Matrix posB ) {
        return false;
    }
};

/**
 * \brief Collision model taking into account only spherical collisions of the
 * shoes
 */
class SimpleColision {
public:
    /**
     * \brief Decide if two modules collide
     */
    bool operator()( const Module& a, const Module& b, Matrix posA, Matrix posB ) {
        for ( auto pA : a.getOccupiedPositions() ) {
            for ( auto pB : b.getOccupiedPositions() ) {
                if ( equals( static_cast< Matrix >( posA * pA ), posB * pB ) )
                    return true;
            }
        }

        return false;
    }
};

/**
 * \brief RoFI bot
 *
 * The rofibot is composed out of modules.
 */
class Rofibot {
    struct ModuleInfo;
    using HandleId = atoms::HandleSet< ModuleInfo >::handle_type;
    using HandleJoint = atoms::HandleSet< RoficomJoint >::handle_type;
public:
    using HandleSpace = atoms::HandleSet< SpaceJoint >::handle_type;

    Rofibot() = default;

    Rofibot( const Rofibot& other )
        : _modules( other._modules ),
          _moduleJoints( other._moduleJoints ),
          _spaceJoints( other._spaceJoints ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    Rofibot& operator=( Rofibot other ) {
        swap( other );
        return *this;
    }

    Rofibot( Rofibot&& other )
        : _modules( std::move( other._modules ) ),
          _moduleJoints( std::move( other._moduleJoints ) ),
          _spaceJoints( std::move( other._spaceJoints ) ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    Rofibot& operator=( Rofibot&& other ) {
        swap( other );
        return *this;
    }

    void swap( Rofibot& other ) {
        using std::swap;
        swap( _modules, other._modules );
        swap( _moduleJoints, other._moduleJoints );
        swap( _spaceJoints, other._spaceJoints );
        swap( _prepared, other._prepared );
        _adoptModules();
        other._adoptModules();
    }

    /**
     * \brief Insert a module from the Rofibot.
     *
     * The module position is not specify. You should connect the module to
     * other modules via connect(). The module is assigned a unique id within
     * the rofibot.
     *
     * Returns a reference to the newly created module.
     */
    Module& insert( const Module& m ) {
        if ( _idMapping.find( m._id ) != _idMapping.end() )
            throw std::runtime_error( "Module with given id is already present" );
        auto id = _modules.insert( { m, {}, {}, {}, std::nullopt } );
        _idMapping.insert( { _modules[ id ].module->_id, id } );
        Module* insertedModule = _modules[ id ].module.get();
        insertedModule->parent = this;
        insertedModule->_prepareComponents();
        return *insertedModule;
    }

    /**
     * \brief Get pointer to module with given id within the Rofibot
     */
    Module* getModule( ModuleId id ) const {
        if ( _idMapping.find( id ) == _idMapping.end() )
            return nullptr;
        auto& ref = *_modules[ _idMapping.at( id ) ].module.get();
        return &ref;
    }

    /**
     * \brief Get a container of ModuleInfo
     */
    const auto& modules() const {
        return _modules;
    }

    /**
     * \brief Get a container of RoficomJoint
     */
    const auto& roficoms() const {
        return _moduleJoints;
    }

    /**
     * \brief Remove a module from the Rofibot
     */
    void remove( ModuleId id ) {
        if ( _idMapping.find( id ) == _idMapping.end() )
            return;
        auto handle = _idMapping[ id ];
        const ModuleInfo& info = _modules[ handle ];
        for ( auto idx : info.inJointsIdx )
            _moduleJoints.erase( idx );
        for ( auto idx : info.outJointsIdx )
            _moduleJoints.erase( idx );
        for ( auto idx : info.spaceJoints )
            _spaceJoints.erase( idx );
        _modules.erase( handle );
        _idMapping.erase( id );
    }

    /**
     * \brief Decided whether the configuration is valid
     *
     * \return A pair - first item indicates the validity, the second one gives
     * textual description of the reason for invalidity
     */
    template < typename Collision >
    std::pair< bool, std::string > isValid( Collision collisionModel = Collision() ) {
        if ( !_prepared ) {
            try {
                prepare();
            }
            catch ( const std::runtime_error& e ) {
                return { false, e.what() };
            }
        }

        for ( const ModuleInfo& m : _modules ) {
            for ( const ModuleInfo& n : _modules ) {
                if ( n.module->_id >= m.module->_id ) // Collision is symmetric
                    break;
                if ( collisionModel( *n.module, *m.module, *n.position, *m.position ) ) {
                    return { false, fmt::format("Modules {} and {} collide",
                        m.module->_id, n.module->_id ) };
                }
            }
        }

        for ( const ModuleInfo& m : _modules ) {
            if ( !m.position )
                return { false, fmt::format("Module {} is not rooted",
                        m.module->_id) };
        }
        return { true, "" };
    }

    /**
     * \brief Precompute position of all the modules in the configuration
     *
     * Raises std::runtime_error if the configuration is inconsistent, and,
     * therefore, it cannot be
     */
    void prepare() {
        _clearModulePositions();

        // Setup position of space joints and extract roots
        std::set< HandleId > roots;
        for ( const SpaceJoint& j : _spaceJoints ) {
            Matrix jointPosition = translate( j.refPoint ) * j.joint->sourceToDest();
            ModuleInfo& mInfo = _modules[ _idMapping[ j.destModule ] ];
            Matrix componentPosition = mInfo.module->getComponentPosition( j.destComponent );
            // Reverse the comonentPosition to get position of the module origin
            Matrix modulePosition = jointPosition * arma::inv( componentPosition );
            if ( mInfo.position ) {
                if ( !equals( mInfo.position.value(), modulePosition ) )
                    throw std::runtime_error(
                        fmt::format( "Inconsistent rooting of module {}", mInfo.module->_id ) );
            } else {
                mInfo.position = componentPosition;
            }
            roots.insert( _idMapping[ mInfo.module->_id ] );
        }

        auto dfsTraverse = [&]( ModuleInfo& m, Matrix position, auto& self ) {
            if ( m.position ) {
                if ( !equals( position, m.position.value() ) )
                    throw std::runtime_error(
                        fmt::format( "Inconsistent position of module {}", m.module->_id ) );
                return;
            }
            m.position = position;
            // Traverse ignoring edge orientation
            std::vector< HandleJoint > joints;
            std::copy( m.outJointsIdx.begin(), m.outJointsIdx.end(),
                std::back_inserter( joints ) );
            std::copy( m.inJointsIdx.begin(), m.inJointsIdx.end(),
                std::back_inserter( joints ) );
            for ( auto outJointIdx : joints ) {
                const RoficomJoint& j = _moduleJoints[ outJointIdx ];

                bool mIsSource = j.sourceModule == m.module->_id;
                Matrix jointTransf = mIsSource ? j.sourceToDest() : j.destToSource();
                Matrix jointRefPosition = position
                                        * m.module->getComponentPosition( mIsSource
                                                                        ? j.sourceConnector
                                                                        : j.destConnector )
                                        * jointTransf;
                ModuleInfo& other = _modules[ _idMapping[ mIsSource ? j.destModule : j.sourceModule ] ];
                Matrix otherConnectorPosition = other.module->getComponentPosition( mIsSource
                                                                                  ? j.destConnector
                                                                                  : j.sourceConnector );
                // Reverse the comonentPosition to get position of the module origin
                Matrix otherPosition = jointRefPosition * arma::inv( otherConnectorPosition );
                self( other, otherPosition, self );
            }
        };

        for ( auto h : roots ) {
            ModuleInfo& m = _modules[ h ];
            auto pos = m.position.value();
            m.position.reset();
            dfsTraverse( m, pos, dfsTraverse );
        }

        _prepared = true;
    }

    /**
     * \brief Set position of a space joints specified by its id
     */
    void setSpaceJointPosition( HandleSpace jointId, const Joint::Positions& p ) {
        assert( p.size() == _spaceJoints[ jointId ]. joint->positions.size() );
        _spaceJoints[ jointId ].joint->positions = p;
        _prepared = false;
    }

    /**
     * \brief Get position of a module specified by its id
     */
    Matrix getModulePosition( ModuleId id ) {
        if ( !_prepared )
            prepare();
        if ( _idMapping.find( id ) == _idMapping.end() )
            throw std::runtime_error( "bad access: rofibot does not containt module with such id" );
        return _modules[ _idMapping[ id ] ].position.value();
    }

private:
    void onModuleMove( ModuleId mId ) {
        _prepared = false;
    }

    void _clearModulePositions() {
        for ( ModuleInfo& m : _modules )
            m.position = std::nullopt;
    }

    void _adoptModules() {
        for ( ModuleInfo& m : _modules )
            m.module->parent = this;
    }

    struct ModuleInfo {
        ModuleInfo( const ModuleInfo& ) = default;
        ModuleInfo( ModuleInfo&& o ) noexcept
        : module( std::move( o.module ) ), inJointsIdx( std::move( o.inJointsIdx ) ),
            outJointsIdx( std::move( o.outJointsIdx ) ), spaceJoints( std::move( o.spaceJoints ) ),
            position( std::move( o.position ) )
        {}

        ModuleInfo( atoms::ValuePtr< Module >&& m, const std::vector< HandleJoint >& i, const std::vector< HandleJoint >& o,
            const std::vector< HandleSpace >& s, const std::optional< Matrix >& pos )
        : module( std::move( m ) ), inJointsIdx( i ), outJointsIdx( o ), spaceJoints( s ), position( pos )
        {}

        ModuleInfo& operator=( const ModuleInfo& ) = default;
        ModuleInfo& operator=( ModuleInfo&& ) = default;

        atoms::ValuePtr< Module > module;  // Use value_ptr to make address of modules stable
        std::vector< HandleJoint > inJointsIdx;
        std::vector< HandleJoint > outJointsIdx;
        std::vector< HandleSpace > spaceJoints;
        std::optional< Matrix > position;
    };

    atoms::HandleSet< ModuleInfo > _modules;
    atoms::HandleSet< RoficomJoint > _moduleJoints;
    atoms::HandleSet< SpaceJoint > _spaceJoints;
    std::map< ModuleId, HandleId > _idMapping;
    bool _prepared = false;

    friend void connect( const Component& c1, const Component& c2, roficom::Orientation o );
    friend class Module;
    template < typename JointT, typename... Args >
    friend HandleSpace connect( const Component& c, Vector refpoint, Args&&... args );
};

/**
 * \brief Connect two modules via connector
 *
 * Requires that both modules belong to the same Rofibot, otherwise
 * std::logic_error is thrown.
 *
 */
void connect( const Component& c1, const Component& c2, roficom::Orientation o );

/**
 * \brief Connect a module's component to a point in space via given joint type
 *
 * First argument specified the component, the rest of the arguments are
 * forwarded
 *
 * Returns ID of the joint which can be used for setting parameters of the joint
 */
template < typename JointT, typename... Args >
Rofibot::HandleSpace connect( const Component& c, Vector refpoint, Args&&... args ) {
    Rofibot& bot = *c.parent->parent;
    Rofibot::ModuleInfo& info = bot._modules[ bot._idMapping[ c.parent->getId() ] ];

    auto jointId = bot._spaceJoints.insert( SpaceJoint{
        std::unique_ptr< Joint >( new JointT( ( std::forward< Args >( args ) )... ) ),
        refpoint,
        info.module->getId(),
        info.module->componentIdx( c )
    } );

    info.spaceJoints.push_back( jointId );
    bot._prepared = false;

    return jointId;
}

} // namespace rofi::configuration
