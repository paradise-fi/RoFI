#pragma once

#include <algorithm>
#include <memory>
#include <vector>
#include <set>
#include <span>
#include <map>
#include <ranges>

#include <atoms/containers.hpp>
#include <atoms/units.hpp>
#include <atoms/util.hpp>
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

    std::string orientationToString( Orientation o );
    Orientation stringToOrientation( const std::string& str );
} // namespace roficom

/**
 * \brief Joint between two components of the same module
 */
struct ComponentJoint {
    ComponentJoint( atoms::ValuePtr< Joint > joint, int source, int dest ):
        joint( std::move( joint ) ),
        sourceComponent( source ),
        destinationComponent( dest )
    {}
    ComponentJoint( ComponentJoint && ) = default;
    ComponentJoint& operator=( ComponentJoint && ) = default;
    ComponentJoint( const ComponentJoint & o ):
        ComponentJoint( o.joint.clone(), o.sourceComponent, o.destinationComponent )
    {}
    ComponentJoint& operator=( const ComponentJoint & o ) {
        *this = ComponentJoint( o );
        return *this;
    }

    atoms::ValuePtr< Joint > joint;
    int sourceComponent, destinationComponent;
};

template< typename JointT, typename...Args >
ComponentJoint makeComponentJoint( int source, int dest, Args&&...args ) {
    return ComponentJoint(
        atoms::ValuePtr< Joint >( std::make_unique< JointT >( ( std::forward< Args >( args ) )... ) ),
        source, dest );
}

struct RoficomJoint;
struct SpaceJoint;

class Rofibot;
class Module;

/**
 * \brief Single body of a module
 */
struct Component {
    using JointId = int;

    Component( ComponentType type,
               std::vector< JointId > inJoints,
               std::vector< JointId > outJoints,
               Module *parent = nullptr ):
        type( type ),
        inJoints( std::move( inJoints ) ),
        outJoints( std::move( outJoints ) ),
        parent( parent )
    {}
    Component( ComponentType type ): Component( type, {}, {} ) {}

    ComponentType type;
    std::vector< JointId > inJoints;
    std::vector< JointId > outJoints;

    Module* parent = nullptr;

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
    : type( type ),
      parent( nullptr ),
      _id( id ),
      _components( std::move( components ) ),
      _connectorCount( connectorCount ),
      _joints( std::move( joints ) ),
      _rootComponent( rootComponent ),
      _componentPosition( std::nullopt )
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

    void setJointPositions( int idx, std::span< const float > p );
    // Implemented in CPP files as it depends on definition of Rofibot

    /**
     * \brief Get a component position relative to module origin
     *
     * Raises std::logic_error if the components are inconsistent
     */
    Matrix getComponentPosition( int idx ) {
        assert( idx >= 0 );
        assert( to_unsigned( idx ) < _components.size() );
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
        assert( idx >= 0 );
        assert( to_unsigned( idx ) < _components.size() );
        if ( !_componentPosition )
            throw std::logic_error( "Module is not prepared" );
        return position * _componentPosition.value()[ idx ];
    }

    void clearComponentPositions() {
        _componentPosition = std::nullopt;
    }

    /**
     * \brief Get a vector of occupied positions relative to module origin
     *
     * Raises std::logic_error if the components are not prepared
     */
    std::vector< Matrix > getOccupiedPositions() const {
        using namespace rofi::configuration::matrices;
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
        using namespace rofi::configuration::matrices;
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

        if ( !std::ranges::all_of( initialized, std::identity{} ) )
            throw std::logic_error( "There are components without position" );
        _componentPosition = std::move( positions );
    }

    auto configurableJoints() {
        return _joints | std::views::transform( []( ComponentJoint& cj ) -> Joint& { return *cj.joint; } )
                       | std::views::filter( []( const Joint& joint ) {
                                                    return joint.positions().size() > 0;
                                                } );
    }

    auto configurableJoints() const {
        return _joints | std::views::transform( []( const ComponentJoint& cj ) -> const Joint& { return *cj.joint; } )
                       | std::views::filter( []( const Joint& joint ) {
                                                    return joint.positions().size() > 0;
                                                } );
    }

    /**
     * \brief Get read-only view of the components
     */
    std::span< const Component > components() const {
        return _components;
    }

    /**
     * \brief Get read-only view of the bodies
     */
    std::span< const Component > bodies() const {
        return components().subspan( _connectorCount );
    }

    std::span< const ComponentJoint > joints() const {
        return _joints;
    }

    /**
     * \brief Get read-only view of the connectors
     */
    std::span< const Component > connectors() const {
        return components().subspan( 0, _connectorCount );
    };

    /**
     * \brief Get index of a component
     */
    int componentIdx( const Component& c ) const {
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
        for ( Component::JointId i = 0; to_unsigned( i ) < _joints.size(); i++ ) {
            const auto& j = _joints[ to_unsigned( i ) ];
            _components[ j.sourceComponent ].outJoints.push_back( i );
            _components[ j.destinationComponent ].inJoints.push_back( i );
        }
    }

    /**
     * \brief Get index of root component - component with no ingoing edges
     */
    int _computeRoot() const {
        for ( size_t i = 0; i < _components.size(); i++ )
            if ( _components[ i ].inJoints.size() == 0 )
                return static_cast< int >( i );
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
    bool operator()( const Module& /* a */, const Module& /* b */, Matrix /* posA */, Matrix /* posB */ ) {
        return false;
    }
};

/**
 * \brief Collision model taking into account only spherical collisions of the
 * shoes
 */
class SimpleCollision {
public:
    /**
     * \brief Decide if two modules collide
     */
    bool operator()( const Module& a, const Module& b, Matrix posA, Matrix posB ) {
        for ( auto pA : a.getOccupiedPositions() ) {
            for ( auto pB : b.getOccupiedPositions() ) {
                if ( rofi::configuration::matrices::equals( static_cast< Matrix >( posA * pA ), posB * pB ) ) {
                    return true;
                }
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
public:
    using ModuleInfoHandle = atoms::HandleSet< ModuleInfo   >::handle_type;
    using RoficomHandle    = atoms::HandleSet< RoficomJoint >::handle_type;
    using SpaceJointHandle = atoms::HandleSet< SpaceJoint   >::handle_type;

    Rofibot() = default;

    Rofibot( const Rofibot& other )
        : _modules( other._modules ),
          _moduleJoints( other._moduleJoints ),
          _spaceJoints( other._spaceJoints ),
          _idMapping( other._idMapping ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    Rofibot( Rofibot&& other )
        : _modules( std::move( other._modules ) ),
          _moduleJoints( std::move( other._moduleJoints ) ),
          _spaceJoints( std::move( other._spaceJoints ) ),
          _idMapping( std::move( other._idMapping ) ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    Rofibot& operator=( Rofibot other ) {
        swap( other );
        return *this;
    }

    void swap( Rofibot& other ) {
        using std::swap;
        swap( _modules, other._modules );
        swap( _moduleJoints, other._moduleJoints );
        swap( _spaceJoints, other._spaceJoints );
        swap( _idMapping, other._idMapping );
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
        if ( _idMapping.contains( m._id ) )
            throw std::runtime_error( "Module with given id is already present" );
        auto id = _modules.insert( { atoms::ValuePtr( m ), {}, {}, {}, std::nullopt } );
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
        if ( !_idMapping.contains( id ) )
            return nullptr;
        return _modules[ _idMapping.at( id ) ].module.get();
    }

    /**
     * \brief Get pointer to module with given id within the Rofibot
     */
    Module* getModule( ModuleInfoHandle h ) const {
        if ( !_modules.contains( h ) )
            return nullptr;
        return _modules[ h ].module.get();
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

    const auto& referencePoints() const {
        return _spaceJoints;
    }

    /**
     * \brief Remove a module from the Rofibot
     */
    void remove( ModuleId id ) {
        if ( !_idMapping.contains( id ) )
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
     * @brief Return true if the configuration is prepared
     * 
     * Configuration can be prepared with `prepare`.
     */
    bool isPrepared() const {
        return _prepared;
    }

    /**
     * \brief Decide whether the configuration is valid
     *
     * \return A pair - first item indicates the validity, the second one gives
     * textual description of the reason for invalidity
     */
    template < typename Collision = SimpleCollision >
    std::pair< bool, std::string > isValid( Collision collisionModel = Collision() ) const {
        if ( !_prepared ) {
            return { false, "configuration is not prepared" };
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
     * \brief Prepare configuration if needed and decide whether it is valid
     *
     * \return A pair - first item indicates the validity, the second one gives
     * textual description of the reason for invalidity
     */
    template < typename Collision = SimpleCollision >
    [[nodiscard]] std::pair< bool, std::string > validate( Collision collisionModel = Collision() ) {
        if ( !_prepared ) {
            try {
                prepare();
            } catch ( const std::runtime_error& err ) {
                return { false, err.what() };
            }
        }
        return isValid( collisionModel );
    }

    /**
     * \brief Precompute position of all the modules in the configuration
     *
     * Raises std::runtime_error if the configuration is inconsistent, and,
     * therefore, it cannot be
     */
    void prepare();

    /**
     * \brief Set position of a space joints specified by its id
     */
    void setSpaceJointPosition( SpaceJointHandle jointId, std::span< const float > p );

    /**
     * \brief Get position of a module specified by its id
     */
    Matrix getModulePosition( ModuleId id ) {
        if ( !_prepared )
            prepare();
        if ( !_idMapping.contains( id ) )
            throw std::runtime_error( "bad access: rofibot does not containt module with such id" );
        return _modules[ _idMapping[ id ] ].position.value();
    }

private:
    void onModuleMove() {
        _prepared = false;
    }

    void _clearModulePositions() {
        for ( ModuleInfo& m : _modules ) {
            m.position = std::nullopt;
            m.module->clearComponentPositions();
        }
    }

    void _adoptModules() {
        for ( ModuleInfo& m : _modules )
            m.module->parent = this;
    }

    struct ModuleInfo {
        ModuleInfo( atoms::ValuePtr< Module > m, std::vector< RoficomHandle > i, std::vector< RoficomHandle > o,
            std::vector< SpaceJointHandle > s, std::optional< Matrix > pos )
        : module( std::move( m ) ), inJointsIdx( std::move( i ) ), outJointsIdx( std::move( o ) ),
            spaceJoints( std::move( s ) ), position( std::move( pos ) )
        {}

        ModuleInfo( ModuleInfo&& o ) noexcept = default;
        ModuleInfo& operator=( ModuleInfo&& ) = default;

        ModuleInfo( const ModuleInfo& o )
        : ModuleInfo( o.module.clone(), o.inJointsIdx, o.outJointsIdx, o.spaceJoints, o.position )
        {}
        ModuleInfo& operator=( const ModuleInfo& o ) {
            this->module = o.module.clone();
            this->inJointsIdx = o.inJointsIdx;
            this->outJointsIdx = o.outJointsIdx;
            this->spaceJoints = o.spaceJoints;
            this->position = o.position;
            return *this;
        }

        atoms::ValuePtr< Module > module;  // Use value_ptr to make address of modules stable
        std::vector< RoficomHandle > inJointsIdx;
        std::vector< RoficomHandle > outJointsIdx;
        std::vector< SpaceJointHandle > spaceJoints;
        std::optional< Matrix > position;
    };

    atoms::HandleSet< ModuleInfo > _modules;
    atoms::HandleSet< RoficomJoint > _moduleJoints;
    atoms::HandleSet< SpaceJoint > _spaceJoints;
    std::map< ModuleId, ModuleInfoHandle > _idMapping;
    bool _prepared = false;

    friend RoficomHandle connect( const Component& c1, const Component& c2, roficom::Orientation o );
    friend class Module;
    template < typename JointT, typename... Args >
    friend SpaceJointHandle connect( const Component& c, Vector refpoint, Args&&... args );
};



/**
 * \brief Joint between two modules.
 *
 * The joint is created between two components of two modules specified by
 * module ids and corresponding component index.
 */
struct RoficomJoint : public Joint {
    RoficomJoint( roficom::Orientation o, Rofibot::ModuleInfoHandle sourceModule, Rofibot::ModuleInfoHandle destModule,
        int sourceConnector, int destConnector)
    : Joint( std::vector< std::pair< float, float > >{} ),
      orientation( o ), sourceModule( sourceModule ), destModule( destModule ),
      sourceConnector( sourceConnector ), destConnector( destConnector )
    {}

    Matrix sourceToDest() const override {
        using namespace rofi::configuration::matrices;
        // the "default" roficom is A-X
        return translate( { -1, 0, 0 } ) * rotate( M_PI, { 0, 0, 1 } )
            * rotate( M_PI, { 1, 0, 0 } )
            * rotate( roficom::orientationToAngle( orientation ), { -1, 0, 0 } );
    }

    ATOMS_CLONEABLE( RoficomJoint );

    roficom::Orientation orientation;
    Rofibot::ModuleInfoHandle sourceModule;
    Rofibot::ModuleInfoHandle destModule;
    int sourceConnector;
    int destConnector;
};

/**
 * \brief Joint between a fixed point in space and a module
 */
struct SpaceJoint {
    SpaceJoint( atoms::ValuePtr< Joint > joint, Vector refPoint
              , Rofibot::ModuleInfoHandle destModule, int destComponent ):
        joint( std::move( joint ) ),
        refPoint( std::move( refPoint ) ),
        destModule( destModule ),
        destComponent( destComponent )
    {}
    SpaceJoint( SpaceJoint && ) = default;
    SpaceJoint& operator=( SpaceJoint && ) = default;
    SpaceJoint( const SpaceJoint & o ):
        SpaceJoint( o.joint.clone(), o.refPoint, o.destModule, o.destComponent )
    {}
    SpaceJoint& operator=( const SpaceJoint & o ) {
        *this = SpaceJoint( o );
        return *this;
    }

    atoms::ValuePtr< Joint > joint;
    Vector refPoint;
    Rofibot::ModuleInfoHandle destModule;
    int destComponent;
};

/**
 * \brief Connect two modules via connector
 *
 * Requires that both modules belong to the same Rofibot, otherwise
 * std::logic_error is thrown.
 *
 */
Rofibot::RoficomHandle connect( const Component& c1, const Component& c2, roficom::Orientation o );

/**
 * \brief Connect a module's component to a point in space via given joint type
 *
 * First argument specified the component, the rest of the arguments are
 * forwarded
 *
 * Returns ID of the joint which can be used for setting parameters of the joint
 */
template < typename JointT, typename... Args >
Rofibot::SpaceJointHandle connect( const Component& c, Vector refpoint, Args&&... args ) {
    Rofibot& bot = *c.parent->parent;
    Rofibot::ModuleInfo& info = bot._modules[ bot._idMapping[ c.parent->getId() ] ];

    auto y = std::unique_ptr< Joint >(std::unique_ptr< JointT >());
    auto jointId = bot._spaceJoints.insert( SpaceJoint(
        atoms::ValuePtr< Joint >( std::make_unique< JointT >( std::forward< Args >( args )... ) ),
        refpoint,
        bot._idMapping[ info.module->getId() ],
        info.module->componentIdx( c )
    ) );

    info.spaceJoints.push_back( jointId );
    bot._prepared = false;

    return jointId;
}

} // namespace rofi::configuration
