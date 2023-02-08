#pragma once

#include <algorithm>
#include <concepts>
#include <memory>
#include <vector>
#include <set>
#include <span>
#include <map>
#include <ranges>

#include <atoms/containers.hpp>
#include <atoms/result.hpp>
#include <atoms/units.hpp>
#include <atoms/util.hpp>
#include <fmt/format.h>

#include <configuration/joints.hpp>

namespace rofi::configuration {

/// ModuleId
using ModuleId = int;

enum class ComponentType {
    Roficom,
    UmBody,
    UmShoe,
    CubeBody,
};

enum class ModuleType {
    Unknown,
    Universal,
    Pad,
    Cube,
};

namespace roficom {
    enum class Orientation { North, East, South, West };

    /**
     * \returns angle corresponding to the given orientation
     */
    Angle orientationToAngle( Orientation o = Orientation::North );
    Matrix orientationToTransform( roficom::Orientation orientation );

    std::string orientationToString( Orientation o );
    atoms::Result< Orientation > stringToOrientation( const std::string & str );
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

class RofiWorld;
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
    explicit Component( ComponentType type ): Component( type, {}, {} ) {}

    ComponentType type;
    std::vector< JointId > inJoints;
    std::vector< JointId > outJoints;

    Module* parent = nullptr;

    bool operator==( const Component& o ) const = default;

    /**
     * \brief Get the index of component in parent
     */
    int getIndexInParent() const;
    /**
     * \brief Get the absolute component position
     *
     * \throws std::logic_error if the world is not prepared
     */
    Matrix getPosition() const;

    /**
     * \brief Find a connector of another module in the same RofiWorld
     *
     * that can be connected to
     *
     * \returns the connector and orientation
     * \returns `nullopt` if no such connector exists
     *
     * \throws std::logic_error if the world is not prepared
     */
    std::optional< std::pair< const Component&, roficom::Orientation > > getNearConnector() const;
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
      _componentRelativePositions( std::nullopt )
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
     * Checks if the new value is not used within its world,
     * in which case the ID is changed to the new value.
     * Otherwise the ID is left the same.
     *
     * \returns whether the value was changed
     */
    bool setId( ModuleId newId );

    void setJointPositions( int idx, std::span< const float > p );
    // Implemented in CPP files as it depends on definition of RofiWorld

    /**
     * \brief Changes parameters of joint positions by values in <diff>.
     * 
     * Assumes <idx> is at most index of last joint in joint container.
     * Assumes <diff> contains as many values as joint has parameters.
     * If values from <diff> added to values of current parameters 
     * are not bounded by pairs of values in joint limits, 
     * function does not change the positions
     * and does not clear component positions 
     * (configuration is still prepared).
     * 
     * \param idx Joint index
     * @param diff Span of values to be added to current parameters (can be negative).
     * @return result_error if any of the resulting parameters 
     * does not respect corresponding joint limit. 
     */
    atoms::Result< std::monostate> changeJointPositionsBy( int idx, std::span< float > diff );

    /**
     * \brief Get a component position relative to module origin
     *
     * \throws std::logic_error if the components are inconsistent
     */
    Matrix getComponentRelativePosition( int idx ) {
        assert( idx >= 0 );
        assert( to_unsigned( idx ) < _components.size() );
        if ( !_componentRelativePositions )
            prepare().get_or_throw_as< std::logic_error >();

        return _componentRelativePositions.value()[ idx ];
    }

    /**
     * \brief Get a component position relative to module origin
     *
     * \throws std::logic_error if the components are not prepared
     */
    Matrix getComponentRelativePosition( int idx ) const {
        assert( idx >= 0 );
        assert( to_unsigned( idx ) < _components.size() );
        if ( !_componentRelativePositions )
            throw std::logic_error( "Module is not prepared" );
        return _componentRelativePositions.value()[ idx ];
    }

    void clearComponentPositions();

    /**
     * \brief Get a vector of occupied positions relative to module origin
     *
     * \throws std::logic_error if the components are not prepared
     */
    std::vector< Matrix > getOccupiedRelativePositions() const {
        using namespace rofi::configuration::matrices;
        if ( !_componentRelativePositions )
            throw std::logic_error( "Module is not prepared" );

        std::vector< Matrix > res;
        for ( auto& m : _componentRelativePositions.value() ) {
            res.push_back( translate( center( m ) ) );
        }
        std::sort( res.begin(), res.end(), []( const Matrix & a, const Matrix & b ) {
            for (int x = 0; x < 4; x++ ) {
                for (int y = 0; y < 4; y++ ) {
                    if ( std::abs( a( x, y ) - b( x, y ) ) > 1 / matrices::precision ) {
                        return a( x, y ) < b( x, y );
                    }
                }
            }
            return false;
        } );
        res.erase( std::unique( res.begin(), res.end(), []( auto a, auto b ) { return equals( a, b ); } ), res.end() );

        return res;
    }

    /**
     * \brief Precompute component relative positions
     *
     * \returns result error if the components are inconsistent
     */
    atoms::Result< std::monostate > prepare() {
        using namespace rofi::configuration::matrices;
        std::vector< Matrix > relPositions( _components.size() );
        std::vector< bool > initialized( _components.size() );

        auto dfsTraverse = [&]( int compIdx, Matrix relPosition, auto& self ) -> atoms::Result< std::monostate >
        {
            if ( initialized[ compIdx ] ) {
                if ( !equals( relPosition, relPositions[ compIdx ] ) ) {
                    return atoms::result_error< std::string >( "Inconsistent component relative positions" );
                }
                return atoms::result_value( std::monostate() );
            }
            relPositions[ compIdx ] = relPosition;
            initialized[ compIdx ] = true;
            for ( int outJointIdx : _components[ compIdx ].outJoints ) {
                const ComponentJoint& j = _joints[ outJointIdx ];
                auto result = self( j.destinationComponent, relPosition * j.joint->sourceToDest(), self );
                if ( !result ) {
                    return result;
                }
            }
            return atoms::result_value( std::monostate() );
        };

        if ( auto result = dfsTraverse( _rootComponent.value(), identity, dfsTraverse ); !result ) {
            return result;
        }

        if ( !std::ranges::all_of( initialized, std::identity{} ) ) {
            return atoms::result_error< std::string >( "There are components without relative position" );
        }
        _componentRelativePositions = std::move( relPositions );
        return atoms::result_value( std::monostate() );
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
    std::span< Component > components() {
        return _components;
    }

    /**
     * \brief Get read-only view of the bodies
     */
    std::span< const Component > bodies() const {
        return components().subspan( _connectorCount );
    }
    std::span< Component > bodies() {
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
    std::span< Component > connectors() {
        return components().subspan( 0, _connectorCount );
    };

    /**
     * \brief Get index of a component
     *
     * \throws std::logic_error if the component doesn't belong to the module
     *
     * \returns index of the first equal component
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
    RofiWorld* parent; ///< pointer to parenting RofiWorld

private:
    ModuleId _id = 0; ///< integral identifier unique within a context of a single module
    std::vector< Component > _components; ///< All module components, first _connectorCount are connectors
    int _connectorCount;
    std::vector< ComponentJoint > _joints;
    std::optional< int > _rootComponent;

    std::optional< std::vector< Matrix > > _componentRelativePositions;

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
     *
     * \throws std::logic_error if the module doesn't have a root component
     *
     * \returns index of the root component
     */
    int _computeRoot() const {
        for ( size_t i = 0; i < _components.size(); i++ )
            if ( _components[ i ].inJoints.size() == 0 )
                return static_cast< int >( i );
        throw std::logic_error( "Module does not have a root component" );
    }

    friend class RofiWorld;
};

/**
 * \brief Collision model ignoring collision
 */
class NoCollision {
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
        for ( auto pA : a.getOccupiedRelativePositions() ) {
            for ( auto pB : b.getOccupiedRelativePositions() ) {
                if ( rofi::configuration::matrices::equals( static_cast< Matrix >( posA * pA ), posB * pB ) ) {
                    return true;
                }
            }
        }

        return false;
    }
};

/**
 * \brief RoFI world
 *
 * The RofiWorld is composed out of modules.
 */
class RofiWorld {
    struct ModuleInfo;
public:
    using ModuleInfoHandle   = atoms::HandleSet< ModuleInfo   >::handle_type;
    using RoficomJointHandle = atoms::HandleSet< RoficomJoint >::handle_type;
    using SpaceJointHandle   = atoms::HandleSet< SpaceJoint   >::handle_type;

    RofiWorld() = default;

    RofiWorld( const RofiWorld& other )
        : _modules( other._modules ),
          _moduleJoints( other._moduleJoints ),
          _spaceJoints( other._spaceJoints ),
          _idMapping( other._idMapping ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    RofiWorld( RofiWorld&& other )
        : _modules( std::move( other._modules ) ),
          _moduleJoints( std::move( other._moduleJoints ) ),
          _spaceJoints( std::move( other._spaceJoints ) ),
          _idMapping( std::move( other._idMapping ) ),
          _prepared( other._prepared )
    {
        _adoptModules();
    }

    RofiWorld& operator=( RofiWorld other ) {
        swap( other );
        return *this;
    }

    void swap( RofiWorld& other ) {
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
     * \brief Insert a module from the RofiWorld
     *.
     *
     * The module position is not specified. You should connect the module to
     * other modules via connect(). The module is assigned a unique id within
     * the world.
     *
     * \throws std::logic_error if module with the same ID is already present
     *
     * \returns reference to the newly created module.
     */
    Module& insert( const Module& m ) {
        if ( _idMapping.contains( m._id ) ) {
            throw std::logic_error( "Module with given id is already present" );
        }
        auto id = _modules.insert( { atoms::ValuePtr( m ), {}, {}, {}, std::nullopt } );
        _idMapping.insert( { _modules[ id ].module->_id, id } );
        Module* insertedModule = _modules[ id ].module.get();
        assert( insertedModule != nullptr );
        insertedModule->parent = this;
        insertedModule->_prepareComponents();
        _prepared = false;
        return *insertedModule;
    }

    /**
     * \brief Insert a module from the RofiWorld
     *.
     *
     * The module position is not specified. You should connect the module to
     * other modules via connect(). The module is assigned a unique id within
     * the world.
     *
     * \returns reference to the newly created module.
     */
    template< std::derived_from< Module > ModuleT >
        requires( !std::is_same_v< ModuleT, Module > )
    ModuleT& insert( const ModuleT& m ) {
        const Module& m_ = m;
        Module& insertedModule = insert( m_ );
        assert( dynamic_cast< ModuleT* >( &insertedModule ) != nullptr );
        return static_cast< ModuleT& >( insertedModule );
    }

    /**
     * \brief Get pointer to module with given id within the RofiWorld
     *
     */
    Module* getModule( ModuleId id ) const {
        if ( !_idMapping.contains( id ) )
            return nullptr;
        return _modules[ _idMapping.at( id ) ].module.get();
    }

    /**
     * \brief Get pointer to module with given id within the RofiWorld
     *
     */
    Module* getModule( ModuleInfoHandle h ) const {
        if ( !_modules.contains( h ) )
            return nullptr;
        return _modules[ h ].module.get();
    }

    /**
     * \brief Get a range of modules
     */
    auto modules() const -> std::ranges::range auto
    {
        return std::views::transform( _modules,
                                      []( const ModuleInfo & moduleInfo ) -> const Module & {
                                          assert( moduleInfo.module );
                                          return *moduleInfo.module;
                                      } );
    }

    /**
     * \brief Get a range of modules
     */
    auto modules() -> std::ranges::range auto
    {
        return std::views::transform( _modules, []( ModuleInfo & moduleInfo ) -> Module & {
            assert( moduleInfo.module );
            return *moduleInfo.module;
        } );
    }

    /**
     * \brief Get a range of modules with their absolute positions
     *
     * `this` has to be prepared.
     */
    auto modulesWithAbsPos() const -> std::ranges::range auto
    {
        assert( isPrepared() && "The world has to be prepared" );
        return std::views::transform( _modules, []( const ModuleInfo & moduleInfo ) {
            assert( moduleInfo.module );
            assert( moduleInfo.absPosition && "Position has to be available if world is prepared" );
            return std::pair< const Module &, Matrix >{ *moduleInfo.module, *moduleInfo.absPosition };
        } );
    }

    /**
     * \brief Get a container of RoficomJoint
     */
    const auto& roficomConnections() const {
        return _moduleJoints;
    }

    const auto& referencePoints() const {
        return _spaceJoints;
    }

    /**
     * \brief Remove a module from the RofiWorld
     *
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
        _prepared = false;
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
     * \brief Decide whether the configuration is valid given the collision model
     *
     * \returns result - the error gives textual description of the reason for invalidity
     */
    template < typename Collision = SimpleCollision >
    atoms::Result< std::monostate > isValid( Collision collisionModel = Collision() ) const {
        if ( !_prepared ) {
            return atoms::result_error< std::string >( "Configuration is not prepared" );
        }

        for ( const ModuleInfo& m : _modules ) {
            for ( const ModuleInfo& n : _modules ) {
                if ( n.module->_id >= m.module->_id ) // Collision is symmetric
                    break;
                if ( collisionModel( *n.module, *m.module, *n.absPosition, *m.absPosition ) ) {
                    return atoms::result_error( fmt::format( "Modules {} and {} collide",
                        m.module->_id, n.module->_id ) );
                }
            }
        }

        for ( const ModuleInfo& m : _modules ) {
            if ( !m.absPosition )
                return atoms::result_error( fmt::format( "Module {} is not rooted",
                        m.module->_id) );
        }
        return atoms::result_value( std::monostate() );
    }

    /**
     * \brief Prepare configuration if needed and decide whether it is valid with given collision model
     *
     * \returns result - the error gives textual description of the reason for invalidity
     */
    template < typename Collision = SimpleCollision >
    atoms::Result< std::monostate > validate( Collision collisionModel = Collision() ) {
        if ( !_prepared ) {
            if ( auto result = prepare(); !result ) {
                return result;
            }
        }
        return isValid( collisionModel );
    }

    /**
     * \brief Precompute position of all the modules in the configuration
     *
     * \returns result error if the configuration is inconsistent
     */
    atoms::Result< std::monostate > prepare();

    /**
     * \brief Set position of a space joints specified by its id
     */
    void setSpaceJointPositions( SpaceJointHandle jointId, std::span< const float > p );

    /**
     * \brief Get position of a module specified by its id
     *
     * \throws std::logic_error if configuration is inconsistent or module with given ID doesn't exist
     */
    Matrix getModulePosition( ModuleId id ) {
        if ( !_prepared )
            prepare().get_or_throw_as< std::logic_error >();
        if ( !_idMapping.contains( id ) )
            throw std::logic_error( "bad access: rofi world does not containt module with such id" );
        return _modules[ _idMapping[ id ] ].absPosition.value();
    }

    void disconnect( RoficomJointHandle h );
    void disconnect( SpaceJointHandle h );

private:
    void onModuleMove() {
        _prepared = false;
    }

    void _clearModulePositions() {
        for ( ModuleInfo& m : _modules ) {
            m.absPosition = std::nullopt;
            assert( m.module );
            m.module->clearComponentPositions();
        }
        _prepared = false;
    }

    void _adoptModules() {
        for ( ModuleInfo& m : _modules ) {
            assert( m.module );
            m.module->parent = this;
            for ( auto& c : m.module->_components ) {
                c.parent = m.module.get();
            }
        }
    }

    struct ModuleInfo {
        ModuleInfo( atoms::ValuePtr< Module > m, std::vector< RoficomJointHandle > i, std::vector< RoficomJointHandle > o,
            std::vector< SpaceJointHandle > s, std::optional< Matrix > pos )
        : module( std::move( m ) ), inJointsIdx( std::move( i ) ), outJointsIdx( std::move( o ) ),
            spaceJoints( std::move( s ) ), absPosition( std::move( pos ) )
        {}

        ModuleInfo( ModuleInfo&& o ) noexcept = default;
        ModuleInfo& operator=( ModuleInfo&& ) = default;

        ModuleInfo( const ModuleInfo& o )
        : ModuleInfo( o.module.clone(), o.inJointsIdx, o.outJointsIdx, o.spaceJoints, o.absPosition )
        {}
        ModuleInfo& operator=( const ModuleInfo& o ) {
            this->module = o.module.clone();
            this->inJointsIdx = o.inJointsIdx;
            this->outJointsIdx = o.outJointsIdx;
            this->spaceJoints = o.spaceJoints;
            this->absPosition = o.absPosition;
            return *this;
        }

        atoms::ValuePtr< Module > module;  // Use ValuePtr to enable copy and make address of modules stable on move
        std::vector< RoficomJointHandle > inJointsIdx;
        std::vector< RoficomJointHandle > outJointsIdx;
        std::vector< SpaceJointHandle > spaceJoints;
        std::optional< Matrix > absPosition;
    };

    atoms::HandleSet< ModuleInfo > _modules;
    atoms::HandleSet< RoficomJoint > _moduleJoints;
    atoms::HandleSet< SpaceJoint > _spaceJoints;
    std::map< ModuleId, ModuleInfoHandle > _idMapping;
    bool _prepared = false;

    friend RoficomJointHandle connect( const Component& c1, const Component& c2, roficom::Orientation o );
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
    RoficomJoint( roficom::Orientation o, RofiWorld::ModuleInfoHandle sourceModule
                , RofiWorld::ModuleInfoHandle destModule, int sourceConnector, int destConnector)
    : Joint( std::vector< std::pair< float, float > >{} ),
      orientation( o ), sourceModule( sourceModule ), destModule( destModule ),
      sourceConnector( sourceConnector ), destConnector( destConnector )
    {}

    Matrix sourceToDest() const override {
        return roficom::orientationToTransform( orientation );
    }

    ATOMS_CLONEABLE( RoficomJoint );

    Module& getSourceModule( RofiWorld& world ) const {
        Module * module_ = world.getModule( sourceModule );
        assert( module_ );
        return *module_;
    }
    const Module& getSourceModule( const RofiWorld& world ) const {
        const Module * module_ = world.getModule( sourceModule );
        assert( module_ );
        return *module_;
    }
    Module& getDestModule( RofiWorld& world ) const {
        Module * module_ = world.getModule( destModule );
        assert( module_ );
        return *module_;
    }
    const Module& getDestModule( const RofiWorld& world ) const {
        const Module * module_ = world.getModule( destModule );
        assert( module_ );
        return *module_;
    }

    roficom::Orientation orientation;
    RofiWorld::ModuleInfoHandle sourceModule;
    RofiWorld::ModuleInfoHandle destModule;
    int sourceConnector;
    int destConnector;
};

/**
 * \brief Joint between a fixed point in space and a module
 */
struct SpaceJoint {
    SpaceJoint( atoms::ValuePtr< Joint > joint, Vector refPoint
              , RofiWorld::ModuleInfoHandle destModule, int destComponent ):
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
    RofiWorld::ModuleInfoHandle destModule;
    int destComponent;
};

/**
 * \brief Connect two modules via connector
 *
 * \throws std::logic_error if components don't belong to the same configuration
 *
 * \returns handle to the connection joint
 */
RofiWorld::RoficomJointHandle connect( const Component& c1, const Component& c2, roficom::Orientation o );

/**
 * \brief Connect a module's component to a point in space via given joint type
 *
 * \tparam JointT connection joint type
 *
 * \param c component to connect
 * \param refpoint reference point in space
 * \param args arguments to forward to \p JointT creation
 *
 * \returns handle to the connection joint
 */
template < typename JointT, typename... Args >
RofiWorld::SpaceJointHandle connect( const Component& c, Vector refpoint, Args&&... args ) {
    static_assert( std::is_base_of_v< Joint, JointT > );
    static_assert( std::is_constructible_v< JointT, Args... > );

    RofiWorld& world = *c.parent->parent;
    RofiWorld::ModuleInfo& info = world._modules[ world._idMapping[ c.parent->getId() ] ];

    auto jointHandle = world._spaceJoints.insert( SpaceJoint(
        atoms::ValuePtr< Joint >( std::make_unique< JointT >( std::forward< Args >( args )... ) ),
        refpoint,
        world._idMapping[ info.module->getId() ],
        info.module->componentIdx( c )
    ) );

    info.spaceJoints.push_back( jointHandle );
    world._prepared = false;

    return jointHandle;
}

} // namespace rofi::configuration
