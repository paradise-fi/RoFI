#include "treeReconfig.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <ranges>
#include <set>
#include <stdexcept>
#include <thread>

#include <rofi_hal.hpp>

using namespace rofi::geometry;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace rofi::reconfiguration;
namespace hal = rofi::hal;


bool disconnect( RofiWorld& w, int src_id, int dest_id, int src_connector ){
    //// --std::cout << "disconnecting " << src_id << " from " << dest_id << " connected by " << src_connector << '\n';
    for( auto it = w.roficomConnections().begin(); it != w.roficomConnections().end(); ++it ){
        auto connection = *it;
        const auto& src = connection.getSourceModule( w );
        const auto& dest = connection.getDestModule( w );
        if( ( src.getId() == src_id && dest.getId() == dest_id ) || ( src.getId() == dest_id && dest.getId() == src_id ) ){
            if( ( src.getId() == src_id && connection.sourceConnector == src_connector )
             || ( dest.getId() == src_id && connection.destConnector == src_connector ) )
            {
                auto handle = it.get_handle();
                w.disconnect( handle );
                return true;
            }
        }
    }
    return false;
}

bool disconnect( RofiWorld& world, Tree* source, Tree* target ){
    assert( source );
    assert( target );

    return disconnect( world, source->id, target->id, source->side * 3 + source->in_connector );
}

void blocking_connect( hal::Connector connector ){
    if ( auto state = connector.getState(); state.position == hal::ConnectorPosition::Extended ) {
        throw std::runtime_error( "connector already extended" );
    }
    auto connectionPromise = std::promise< void >();

    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( hal::Connector, hal::ConnectorEvent event ) {
        if ( event == hal::ConnectorEvent::Connected ) {
            std::call_once( onceFlag, [ & ]() { connectionPromise.set_value(); } );
        }
    } );

    connector.connect();

    connectionPromise.get_future().get();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );
}

void blocking_disconnect( hal::Connector connector ){
    if ( auto state = connector.getState(); !state.connected ) {
        throw std::runtime_error( "connector not connected to another module" );
    }
    auto connectionPromise = std::promise< void >();

    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( hal::Connector, hal::ConnectorEvent event ) {
        if ( event == hal::ConnectorEvent::Disconnected ) {
            std::call_once( onceFlag, [ & ]() { connectionPromise.set_value(); } );
        }
    } );

    connector.disconnect();

    connectionPromise.get_future().get();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );

    if( auto state = connector.getState(); state.connected ){
        blocking_disconnect( connector );
    }
}

int connector_mapping( Tree* source, Tree* target, int connector ){
    return connector;
    //// --std::cout << "mapping " << source->id << ", " << source->side << " to " << target->id << ", " << target->side << '\n';
    if( source->side == target->side || connector > 1 ){
        return connector;
    }
    return connector == 0 ? 1 : 0;
}


bool match_connection( const RofiWorld& world, const RofiWorld& target_world, Tree* s_node, Tree* t_node, int child, int mapped ){
    auto s_connection = get_connection( world, s_node->id, s_node->side * 3 + child );
    auto t_connection = get_connection( target_world, t_node->id, t_node->side * 3 + mapped );
    if( child != mapped ){
        if( t_connection.getSourceModule( target_world ).getId() == t_node->id ){
            t_connection.sourceConnector = t_node->side * 3 + child;
        } else {
            t_connection.destConnector = t_node->side * 3 + child;
        }
    }
    return symmetric( s_connection, t_connection );
}

bool rofi::reconfiguration::connected( const RofiWorld& world, int id, int connector ){
    for( const auto& connection : world.roficomConnections() ){
        const auto& src = connection.getSourceModule( world );
        const auto& dest = connection.getDestModule( world );

        if( ( src.getId() == id && connection.sourceConnector == connector ) || ( dest.getId() == id && connection.destConnector == connector ) ){
            return true;
        }
    }
    return false;
}

RoficomJoint rofi::reconfiguration::get_connection( const RofiWorld& world, int id, int connector ){
    for( const auto& connection : world.roficomConnections() ){
        const auto& src = connection.getSourceModule( world );
        const auto& dest = connection.getDestModule( world );

        if( ( src.getId() == id && connection.sourceConnector == connector ) || 
            ( dest.getId() == id && connection.destConnector == connector ) )
        {
            return connection;
        }
    }
    assert( false );
}


void TreeConfiguration::make_tree( const RofiWorld& world, int center, bool side ){
    std::deque< std::tuple< int, bool, int, int, roficom::Orientation, Tree* > > queue;
    std::vector< bool > visited( world.modules().size() * 4, false );
    // // --std::cout << "origin: " << origin << '\n';  
    tree = Tree( origin, false );
    queue.emplace_back( origin, side, -1, -1, roficom::Orientation::North, nullptr );
    visited[ origin * 2 + side ] = true;
    visited[ origin * 2 + !side ] = true;

    while( !queue.empty() ){
        auto [ id, side, connector, in_connector, ori, previous ] = queue.front();
        // // --std::cout << "id: " << id << " side: " << side << " parent: " << ( previous == nullptr ? -1 : previous->id ) << 
            // " in_conn: " << in_connector << '\n';
        queue.pop_front();

        std::shared_ptr< Tree > current = std::make_shared< Tree >( id, side, in_connector, ori, previous );
        auto* raw = current.get();
        if( previous ){
            // // --std::cout << "attaching to connector " << connector << '\n';
            previous->children[ connector ] = std::move( current );
        } else {
            raw = &tree;
        }

        if( !previous || previous->id != id ){
            queue.emplace_back( id, !side, 3, 3, roficom::Orientation::North, raw );
            visited[ id * 2 + int( !side ) ] = true;
        }

        for( const auto& connection : world.roficomConnections() ){
            const auto& src = connection.getSourceModule( world );
            const auto& dest = connection.getDestModule( world );
    
            if( src.getId() == id || dest.getId() == id ){
                // //// --std::cout << "connection: " << src.getId() << "-" << dest.getId() << " src_c: " << connection.sourceConnector << " dest: " << connection.destConnector << '\n';
                int next_id = src.getId() == id ? dest.getId() : src.getId();
                int connector = src.getId() == id ? connection.sourceConnector : connection.destConnector;
                if( ( connector >= 3 ) != side ){
                    continue;
                }
                int next_connector = src.getId() == id ? connection.destConnector : connection.sourceConnector;
                
                bool next_side = next_connector >= 3;
                if( !visited[ next_id * 2 + next_side ] ){
                    queue.emplace_back( next_id, next_side, connector % 3, next_connector % 3, connection.orientation, raw );
                    visited[ next_id * 2 + next_side ] = true;
                    visited[ next_id * 2 + !next_side ] = true;
                }
            }
        }
    }
}

void TreeConfiguration::add_redundant_connections( const RofiWorld& original ){
    for( const auto& connection : original.roficomConnections() ){
        const auto& src = connection.getSourceModule( original );
        const auto& dest = connection.getDestModule( original );
        auto src_connector = connection.sourceConnector;
        auto dest_connector = connection.destConnector;

        auto* src_node = get_node( &tree, src.getId(), src_connector / 3 );
        auto* dest_node = get_node( &tree, dest.getId(), dest_connector / 3 );

        if( depth( src_node ) > depth( dest_node ) ){
            std::swap( src_node, dest_node );
            std::swap( src_connector, dest_connector );
        }
        if( !src_node->children[ src_connector % 3 ] ){
            // //// --std::cout << "adding child " << dest_node->id << " to " << src_node->id << ", child " << src_connector << '\n';
            src_node->children[ src_connector % 3 ] = std::make_shared< Tree >( *dest_node );
        }
    }
    // exit( 1 );
}

double rofi::reconfiguration::difference( const RofiWorld& w1, const RofiWorld& w2 ){
    double diff = 0.0;
    for( const auto& module : w1.modules() ){
        if( module.type != ModuleType::Universal ){
            continue;
        }
        const auto& um = static_cast< const UniversalModule& >( module );
        const auto* other = static_cast< const UniversalModule* >( w2.getModule( um.getId() ) );
        diff += std::fabs( static_cast< double >( um.getAlpha().deg() - other->getAlpha().deg() ) );
        diff += std::fabs( static_cast< double >( um.getBeta().deg() - other->getBeta().deg() ) );
        diff += std::fabs( static_cast< double >( um.getGamma().deg() - other->getGamma().deg() ) );
    }
    return diff;
}

RofiWorld rofi::reconfiguration::from_tree( const Tree& tree, const Matrix& root, bool ignore_first ){
    RofiWorld result;
    auto& um = result.insert( UniversalModule( tree.id, 0_deg, 0_deg, 0_deg ) );
    auto add_connections = [&]( auto* node, auto& self ) -> void {
        for( int i = 0; i < 3; i++ ){
            if( node->children[ i ] ){
                if( !result.getModule( node->id ) ){
                    result.insert( UniversalModule( node->id, 0_deg, 0_deg, 0_deg ) );
                }
                if( !result.getModule( node->children[ i ]->id ) ){
                    result.insert( UniversalModule( node->children[ i ]->id, 0_deg, 0_deg, 0_deg ) );
                }

                if( node->children[ i ]->id != node->id ){
                    auto in_component = result.getModule( node->id )->components()[ node->side * 3 + i ];
                    auto out_component = result.getModule( node->children[ i ]->id )->components()[
                        node->children[ i ]->side * 3 + node->children[ i ]->in_connector ];
                    connect( in_component, out_component, node->children[ i ]->orientation );
                }
                self( node->children[ i ].get(), self );
            }
        }
        
        if( node->children[ 3 ] )
            self( node->children[ 3 ].get(), self );
    };

    auto* start = ignore_first && tree.children[ 3 ] ? tree.children[ 3 ].get() : &tree;
    add_connections( start, add_connections );

    int compIdx = 0;
    if( std::any_of( tree.children.begin(), tree.children.end(), [&]( const auto& child ){ return child && child->id == tree.id; } ) ){
        if( tree.side == 0 ){
            compIdx = 6;
        } else {
            compIdx = 9;
        }
    } else {
        if( tree.side == 0 ){
            compIdx = 7;
        } else {
            compIdx = 8;
        }
    }
    connect< RigidJoint >( um.components()[ compIdx ], Vector{ 0, 0, 0 }, root );

    return result;
}

void TreeConfiguration::make_collision_tree(){
    collision_tree.clear();
    for( const auto& [ module, pos ] : world.modulesWithAbsPos() ){
        if( module.type == ModuleType::Universal ){
            const auto& um = static_cast< const UniversalModule& >( module );
            // // --std::cout << "module " << um.getId() << " inserting:\n";
            collision_tree.insert( Sphere( center( um.getBodyA().getPosition() ) ) );
            // // --std::cout << center( um.getBodyA().getPosition() ) << '\n';
            collision_tree.insert( Sphere( center( um.getBodyB().getPosition() ) ) );
            // // --std::cout << center( um.getBodyB().getPosition() ) << '\n';
        }
        if( module.type == ModuleType::Cube ){
            collision_tree.insert( Sphere( center( pos ) ) );
        }
    }
}

void TreeConfiguration::initialize_trees(){
    make_tree( world, tree.id );
    leaves.clear();
    tree.map( [&]( auto& node ){
        if( std::ranges::all_of( node.children, []( const auto& child ){ return child == nullptr; } ) ){
            leaves.push_back( &node );
        }
    });
    collision_tree.clear();
    //// --std::cout << "world: " << serialization::toJSON( world ) << '\n';
    world.prepare().get_or_throw_as< std::logic_error >();
    make_collision_tree();
}

TreeConfiguration::TreeConfiguration( const RofiWorld& w, int center, bool sim, std::vector< hal::RoFI > rs, int max_length ) : world( w ), origin( center ), simulate( sim ), rofis( rs ), max_length( max_length ) {
    initialize_trees();
    RofiWorld treefied = from_tree( tree, identity );
    copy_angles( w, treefied );
    world = treefied;

    if( simulate && rs.empty() ){
        for( size_t id = 0; id < world.modules().size(); id++ ){
            if( world.getModule( id )->type == ModuleType::Universal ){
                rofis.push_back( hal::RoFI::getRemoteRoFI( id ) );
            }
        }
    }
}

bodies TreeConfiguration::init_arm( Tree* leaf, Tree* base ){
    bodies arm;
    bool seen = false;
    for( auto* node = leaf; node && ( !seen || node->id == base->id ); node = node->parent ){
        auto* um = static_cast< UniversalModule* >( world.getModule( node->id ) );
        Vector pos = center( node->side == 0 ? um->getBodyA().getPosition() : um->getBodyB().getPosition() );

        arm.emplace_front( node->id, node->side == 0 ? A : B );
        arm.front().leaf = collision_tree.find_leaf( Sphere( pos ) );
        assert( leaf );

        if( node == base ){
            seen = true;
        }
    }
    return arm;
}

Matrix TreeConfiguration::position( const Tree* node ) const {
    assert( node != nullptr );
    auto* um = static_cast< UniversalModule* >( world.getModule( node->id ) );
    assert( um != nullptr );
    return node->side == 0 ? um->getBodyA().getPosition() : um->getBodyB().getPosition();
}

Matrix TreeConfiguration::shoe_position( const Tree* node ) const {
    assert( node != nullptr );
    auto* um = static_cast< UniversalModule* >( world.getModule( node->id ) );
    assert( um != nullptr );
    return node->side == 0 ? um->components()[ 6 ].getPosition() : um->components()[ 9 ].getPosition();
}

void TreeConfiguration::straighten_branch( Tree* leaf ){
    auto* shoulder = get_shoulder( leaf );
    RofiWorld copy = world;
    while( leaf != shoulder ){
        if( leaf->side == 0 ){
            static_cast< UniversalModule* >( copy.getModule( leaf->id )  )->setAlpha( 0_deg );
        } else {
            static_cast< UniversalModule* >( copy.getModule( leaf->id )  )->setBeta( 0_deg );
        }
        static_cast< UniversalModule* >( copy.getModule( leaf->id )  )->setGamma( 0_deg );
        copy.prepare().get_or_throw_as< std::logic_error >();
        auto valid = copy.isValid();
        if( valid ){
            world = copy;
            leaf = leaf->parent;
        } else {
            return;
        }
    }
}

std::string reconnect_module( RofiWorld world, int origin, Tree* source, Tree* target, int source_connector = 2, 
    int target_connector = 2, roficom::Orientation orientation = roficom::Orientation::North )
{
    int in_idx = source->side * 3 + source_connector;
    int out_idx = target->side * 3 + target_connector;
    disconnect( world, target->id, target->parent->parent->id, target->parent->side * 3 + target->parent->in_connector );
    rofi::configuration::connect( world.getModule( source->id )->components()[ in_idx ], 
                                  world.getModule( target->id )->components()[ out_idx ],
                                  orientation );
    TreeConfiguration t( world, origin );
    return serialize( t.tree );
}

std::vector< RofiWorld > TreeConfiguration::reach( Tree* leaf, Tree* target, int source_connector, int target_connector,
                                                   roficom::Orientation orientation, const RofiWorld* disconnected )
{
    make_collision_tree();
    auto* base = get_shoulder( leaf, disconnected ? max_length * 2 : max_length );
    auto* node = leaf;
    while( node->parent && node != base ){
        if( node->parent == target && node->parent->children[ 3 ] == nullptr ){
            base = node;
            break;
        }
        if( node == target ){
            base = node;
            break;
        }
        node = node->parent;
    }
    auto arm = init_arm( leaf, base );
    Matrix origin = arm[ 0 ].id == arm[ 1 ].id ? shoe_position( base ) : position( base );
    RofiWorld copy = from_tree( *base, origin, true );
    copy_angles( world, copy );
    Manipulator manipulator( copy, arm, &collision_tree );
    node = leaf;
    for( size_t i = manipulator.arm.size(); i --> 0; node = node->parent ){
        manipulator.arm[ i ].leaf = collision_tree.find_leaf( Sphere( center( position( node ) ) ) );
        // //// --std::cout << "leaf:\n" << center( position( node ) ) << '\n';
    }
    if( disconnected ){
        manipulator.connecting = true;
    }
    //// --std::cout << "manipulator: " << serialization::toJSON( copy ) << '\n';

    int in_idx = leaf->side * 3 + source_connector;
    int out_idx = target->side * 3 + target_connector;

    RofiWorld dummy = world;
    dummy.insert( UniversalModule( -1, 0_deg, 0_deg, 0_deg ) );
    rofi::configuration::connect( dummy.getModule( -1 )->components()[ in_idx ], 
             dummy.getModule( target->id )->components()[ out_idx ],
             orientation );
    auto res = dummy.prepare();
    assert( res );
    //// --std::cout << "dummy: " << serialization::toJSON( dummy ) << '\n';

    Matrix target_position = dummy.getModule( -1 )->components()[ leaf->side * 3 ].getPosition();
    auto result = manipulator.reach( target_position, disconnected );

    if( !result.empty() && simulate ){
        //// --std::cout << "Moving\n";
        // Reset the position
        Manipulator m( from_tree( *base, origin ), arm, &collision_tree, false );
        getchar();
        for( const auto& position : result ){
            simulate_movement( m, position );
            m.world = position;
            auto res = m.world.prepare();
            assert( res );
        }
    }

    if( !result.empty() ){
        copy_angles( result.back(), world );
        //// --std::cout << "success\n" << "worlds:\n";
        for( const auto& w : result ){
            //// --std::cout << serialization::toJSON( w ) << '\n' << std::flush;
        }
    } else {
        make_collision_tree();
    }
    return result;
}

std::vector< RofiWorld > TreeConfiguration::connect_leaves( Tree* leaf, Tree* target, int source_connector, int target_connector, 
                                                roficom::Orientation orientation, bool connect )
{
    // --std::cout << "connecting " << leaf->id << ", " << leaf->side << " with " << target->id << ", " << target->side << " via connectors " << source_connector << " and " << target_connector << '\n';
    // getchar();
    if( simulate ){
        auto rofi = rofis[ leaf->id ];
        auto connector = rofi.getConnector( leaf->side * 3 + source_connector );
        // rofis[ target->id ].getConnector( target->side * 3 + target_connector ).disconnect();
        connector.disconnect();
    }

    auto leaf_connector = world.getModule( leaf->id )->components()[ leaf->side * 3 + source_connector ];
    auto t_connector = world.getModule( target->id )->components()[ target->side * 3 + target_connector ];
    std::vector< RofiWorld > result;
    if( leaf_connector.getNearConnector().has_value() && leaf_connector.getNearConnector().value().first == t_connector 
        && leaf_connector.getNearConnector().value().second == orientation )
    {
        result = { world };
    } else {
        result = !connect ? reach( leaf, target, source_connector, target_connector, orientation )
                : approach_leaves( leaf, target, source_connector, target_connector, orientation );
    }
    if( !result.empty() ){
        rofi::configuration::connect( leaf_connector, t_connector,
                                    orientation );
        connections++;
        for( size_t i = 1; i < result.size(); i++ ){
            movement += difference( result[ i - 1 ], result[ i ] );
        }
        // --std::cout << "after connecting: " << serialization::toJSON( world ) << '\n';
        
        // getchar();
        if( simulate ){
            auto rofi = rofis[ leaf->id ];
            auto connector = rofi.getConnector( leaf->side * 3 + source_connector );
            // connector.connect();
            rofis[ target->id ].getConnector( target->side * 3 + target_connector ).connect();
            std::this_thread::sleep_for( 10ms );
            // connector.disconnect();
            // blocking_connect( rofis[ target->id ].getConnector( target->side * 3 + target_connector ) );
            blocking_connect( connector );
        }
        // getchar();
    }
    return result;
}

void TreeConfiguration::adjust_connectors( Tree* leaf, Tree* target, int source_connector, [[maybe_unused]] int target_connector ){
    auto adjust = [&]( Tree* current, Tree* other ){
        Vector local = center( inverse( position( current ) ) * position( other ) );
        double desired = azimuth( local );

        auto* um = static_cast< UniversalModule* >( world.getModule( current->id ) );
        auto p = um->connectors()[ current->side * 3 + source_connector ].getPosition();
        double rotation = azimuth( inverse( position( current ) ) * p * -Z ) + M_PI_2;
        double diff = desired - rotation;
        if( diff > M_PI_2 ){
            diff -= M_PI;
        }
        if( diff < -M_PI_2 ){
            diff += M_PI;
        }
        auto deg = Angle::rad( float( um->getGamma().rad() - diff ) );
        um->setGamma( deg );

        local = project( X, Vector{ 0, 0, 0, 1 }, local );
        local = rotate( M_PI, Y ) * local;
        auto [ pol, _ ] = simplify( polar( local ), azimuth( local ) );
        um->setBeta( Angle::rad( float( -pol ) ) );
        auto res = world.prepare();
        assert( res );
    };

    adjust( leaf, target );
    adjust( target, leaf );
    //// --std::cout << "after adjustment: " << serialization::toJSON( world );
    
}

std::vector< RofiWorld > TreeConfiguration::approach_leaves( Tree* leaf, Tree* target, int source_connector, int target_connector,
                                                   roficom::Orientation orientation )
{
    // int leaf_size = depth( leaf ) - depth( get_shoulder( leaf ) );
    // int target_size = depth( target ) - depth( get_shoulder( target ) );
    if( get_shoulder( target )->id == tree.id /* || leaf_size < target_size */ ){
        std::swap( leaf, target );
        std::swap( source_connector, target_connector );
    }
    RofiWorld copy = world;
    rofi::configuration::connect( copy.getModule( leaf->id )->components()[ leaf->side * 3 + source_connector ],
                                copy.getModule( target->id )->components()[ target->side * 3 + target_connector ],
                                orientation );
    Tree* s = get_shoulder( target );
    Tree* node = leaf;
    std::vector< int > leaf_ids;
    Tree* leaf_shoulder = get_shoulder( leaf );
    while( node != leaf_shoulder ){
        if( !leaf_ids.empty() && leaf_ids.back() != node->id ){
            leaf_ids.push_back( node->id );
        }
        node = node->parent;
    }
    // if( std::find( leaf_ids.begin(), leaf_ids.end(), prev->id ) != leaf_ids.end() ){
        // for( auto id : leaf_ids ){
        //     //// --std::cout << "id: " << id << '\n';
        // }
        Tree* prev = target;
        while( prev->parent && std::find( leaf_ids.begin(), leaf_ids.end(), prev->parent->id ) == leaf_ids.end() 
                && prev != s && prev->in_connector > 1 
            )
        {
            prev = prev->parent;
        }
    // }
    while( prev->parent && prev->id == prev->parent->id ){
        prev = prev->parent;
    }
    Tree* p = prev->parent;
    //// --std::cout << "prev: " << prev->id << ',' << prev->side << '\n';
    assert( p );
    int src_c = 0;
    for( int i = 0; i < 4; i++ ){
        if( p->children[ i ] && p->children[ i ]->id == prev->id ){
            src_c = i;
        }
    }
    int dest_c = prev->in_connector;
    auto ori = prev->orientation;
    for( int i = 0; i < 3; i++ ){
        if( prev->children[ i ] ){
            disconnect( copy, prev->id, prev->children[ i ]->id, prev->side * 3 + i );
            Matrix origin = shoe_position( prev->children[ i ].get() );
            int compIdx = prev->children[ i ]->side == 0 ? 6 : 9;
            rofi::configuration::connect< RigidJoint >( copy.getModule( prev->children[ i ]->id )->components()[ compIdx ], Vector{ 0, 0, 0 }, origin );
        }
    }

    disconnect( copy, prev->id, p->id, dest_c + prev->side * 3 );

    // auto* shoulder = get_shoulder( leaf );
    // Matrix origin = shoe_position( shoulder );
    // int compIdx = shoulder->side == 0 ? 6 : 9;
    // //// --std::cout << "origin:\n" << origin << "position:\n" << world.getModule( prev->id )->components()[ compIdx ].getPosition() << '\n';
    // rofi::configuration::connect< RigidJoint >( copy.getModule( shoulder->id )->components()[ compIdx ], Vector{ 0, 0, 0 }, origin );
    // //// --std::cout << serialization::toJSON( copy ) << '\n' << std::flush;

    TreeConfiguration tree_copy( copy, origin, simulate, rofis );
    tree_copy.dodge = dodge;
    auto* src = get_node( &tree_copy.tree, prev->id, prev->side );
    auto* dest = get_node( &tree_copy.tree, p->id, p->side);
    // auto ori = prev->children[ src_c ]->orientation;
    if( src == nullptr || dest == nullptr ){
        return approach_leaves( target, leaf, target_connector, source_connector, orientation );
    }
    auto result = tree_copy.reach( src, dest, dest_c, src_c, ori, &world );
    // auto* s = get_shoulder( src );

    if( !result.empty() ){
        copy_angles( result.back(), world );
        // auto adjustment = reach( leaf, target, source_connector, target_connector, orientation );
        // copy_angles( adjustment.back(), world );
        // result.push_back( world );

        //// --std::cout << "copy: " << serialization::toJSON( tree_copy.world ) << '\n';
        //// --std::cout << "ready to connect: " << serialization::toJSON( world ) << '\n';
    }

    return result;
}

std::vector< RofiWorld > TreeConfiguration::fetch_module( Tree* target_node, Tree& target_tree )
{
    std::vector< RofiWorld > result;
    auto extra = extra_leaves( target_node, target_tree );
    auto ls = leaves;
    std::ranges::sort( extra, [&]( auto* l1, auto* l2 ){ 
        return node_distance( get_shoulder( l1 ), get_shoulder( l2 ) ) > node_distance( get_shoulder( l2 ), get_shoulder( target_node ));
    } );
    std::ranges::sort( ls, [&]( auto* l1, auto* l2 ){ 
        return node_distance( get_shoulder( l1 ), get_shoulder( l2 ) ) < node_distance( get_shoulder( l2 ), get_shoulder( target_node ));
    } );
    
    for( auto* target : extra ){
        // auto* closest = closest_leaf( leaf, { leaf->id } );
        for( auto* leaf : ls ){
            //// --std::cout << serialization::toJSON( world );
            if( target == leaf || is_parent( target, leaf ) || arm_length( leaf ) > max_length + 1
            //    || node_distance( get_shoulder( leaf ), target_node ) > node_distance( get_shoulder( target ), target_node )
            )
            {
                continue;
            }
            // if( leaf->parent->children[ 0 ] || leaf->parent->children[ 1 ] || leaf->parent->children[ 2 ] ){
            //     continue;
            // }
            
            //// --std::cout << "target: " << target->id << ", " << target->side << " source: " << leaf->id << ", " << leaf->side << '\n';
            auto reconnected = reconnect_module( world, origin, leaf, target );

            if( visited.contains( reconnected ) ){
                // --std::cout << "visited\n";
                continue;
            }
            
            result = connect_leaves( leaf, target, 2, 2, roficom::Orientation::North, true );
            //// --std::cout << "empty? " << result.empty() << '\n';
            if( !result.empty() ){
                // visited.insert( reconnected );
                auto* to_disconnect = target->parent;
                // std::vector< Tree* > shoulder;
                // for( auto* node = leaf; node != get_shoulder( leaf ); node = node->parent ){
                //     shoulder.push_back( node );
                // }
                // while( !get_node( &target_tree, get_path( to_disconnect->parent ) ) 
                //         && std::ranges::find( shoulder, to_disconnect->parent ) == shoulder.end() ){
                //     to_disconnect = to_disconnect->parent;
                // }
                // if( to_disconnect->parent->id == to_disconnect->id ){
                //     to_disconnect = to_disconnect->parent;
                // }
                disconnect( world, to_disconnect, to_disconnect->parent );
                // --std::cout << "after disconnecting: " << serialization::toJSON( world ) << '\n' << std::flush;
                if( simulate ){
                    auto rofi = rofis[ to_disconnect->id ];
                    blocking_disconnect( rofi.getConnector( to_disconnect->side * 3 + to_disconnect->in_connector ) );
                }
                // getchar();
                initialize_trees();
                //// --std::cout << "returning\n";
                return result;
            }
        }
    }
    return result;
}

std::vector< RofiWorld > TreeConfiguration::remove_module( Tree* target_node, Tree& target_tree ){
    //// --std::cout << "removing module: " << target_node->id << '\n';
    std::vector< RofiWorld > result;
    auto targets = get_leaves( target_node );
    for( auto* target : targets ){
        if( node_distance( target_node, target ) > max_length ){
            return {};
        }
    }
    for( auto* leaf : leaves ){
        for( auto* target : targets ){
            //// --std::cout << "leaf: " << leaf->id << " target: " << target->id << '\n';
            auto* node = leaf;
            while( node->parent && node != target_node ){
                node = node->parent;
            }
            if( node == target_node ){
                continue;
            }
            if( node_distance( target_node, target ) + node_distance( leaf, get_shoulder( leaf ) ) > max_length + 1 ){
                return {};
            }        
            RofiWorld state = world;
            result = connect_leaves( leaf, target, 2, 2, roficom::Orientation::North, true );
            // if( !result.empty() && visited.contains( serialize( tree ) ) ){
            //     world = state;
            //     connections--;
            //     continue;
            // }
            if( !result.empty() ){
                disconnect( world, target_node, target_node->parent );
                // --std::cout << "after disconnecting: " << serialization::toJSON( world ) << '\n' << std::flush;
                if( simulate ){
                    auto rofi = rofis[ target_node->id ];
                    blocking_disconnect( rofi.getConnector( target_node->side * 3 + target_node->in_connector ) );
                }
                // getchar();
                initialize_trees();
                return result;
            }
        }
    }
    return result;
}

std::vector< RofiWorld > TreeConfiguration::reconnect_defect( Tree* defect ){
    std::vector< Tree* > nodes;
    std::vector< RofiWorld > result;
    tree.map( [&]( auto& node ){
        nodes.push_back( &node );
    });
    std::ranges::reverse( nodes );
    for( auto* node : nodes ){
        if( node == defect ){
            continue;
        }
        for( int i = 0; i < 3; i++ ){
            if( !node->children[ i ] && node->in_connector != i ){
                for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::West, roficom::Orientation::East } ){
                     result = connect_leaves( defect, node, 2, i, ori, false );
                     if( !result.empty() ){
                        disconnect( world, defect->parent, defect->parent->parent );
                        if( simulate ){
                            auto rofi = rofis[ defect->id ];
                            blocking_disconnect( rofi.getConnector( defect->parent->side * 3 + defect->parent->in_connector ) );
                        }
                        initialize_trees();
                        return result;
                     }
                }
            }
        }
    }
}

void TreeConfiguration::simulate_movement( const Manipulator& manipulator, const RofiWorld& next, bool constant )
{
    auto diffs = manipulator.relative_changes( next );
    if( constant ){
        for( size_t i = 0; i < diffs.size(); i++ ){
            const auto& body = manipulator.arm[ i ];
            auto j = body.side == A ? Alpha : Beta;
            diffs[ i ] = std::fabs( get_rotation( next, body.id, j ).rad() - get_rotation( manipulator.world, body.id, j ).rad() );

            if( i != 0 && manipulator.arm[ i ].id == manipulator.arm[ i - 1 ].id ){
                diffs[ i ] += std::fabs( get_rotation( next, body.id, Gamma ).rad() - get_rotation( manipulator.world, body.id, Gamma ).rad() );
            }
        }
    }
    double max_diff = *std::max_element( diffs.begin(), diffs.end() );
    std::vector< bool > promises;//( shoulder.size() * 3, false );

    size_t promise_idx = 0;
    for( size_t i = 0; i < manipulator.arm.size(); i++ ){
        const auto& body = manipulator.arm[ i ];
        // //// --std::cout << "id: "<< body.id << " side: "<< body.side << '\n';
        auto it = std::find_if( rofis.begin(), rofis.end(), [&](const auto& rofi){ return rofi.getId() == body.id; } );
        assert( it != rofis.end() );
        auto rofi = *it;
        auto j = body.side == A ? Alpha : Beta;

        float speed = diffs[ i ] / max_diff;

        promises.emplace_back( false );
        rofi.getJoint( j ).setPosition( get_rotation( next, body.id, j ).rad(), std::max( 0.2f, speed ),
                                        [&, promise_idx]( hal::Joint ){ promises[ promise_idx ] = true; } );
        ++promise_idx;

        if( i != 0 && manipulator.arm[ i ].id == manipulator.arm[ i - 1 ].id ){
            promises.emplace_back( false );
            rofi.getJoint( Gamma ).setPosition( get_rotation( next, body.id, Gamma ).rad(), std::max( 0.2f, speed ), 
                                            [&, promise_idx]( hal::Joint ){ promises[ promise_idx ] = true; } );
            ++promise_idx;
        }
    }
    while( std::any_of( promises.begin(), promises.end(), []( bool finished ){ return !finished; } ) ){
        std::this_thread::sleep_for(10ms);
    }

}

Tree TreeConfiguration::intermediate_tree( Tree& source, Tree& target ){
    // std::map< std::pair< int, int >, std::pair< int, int > > mapping;
    std::deque< std::tuple< int, int, std::vector< int > > > queue;

    Tree result( 0, 0 );
    result.children[ 3 ] = std::make_shared< Tree >( 0, 1 );
    size_t inserted = 1;
    queue.emplace_back( 0, 0, std::vector< int >{} );
    queue.emplace_back( 0, 1, std::vector< int >{ 3 } );
    std::set< int > seen = { 0 };

    auto copy_node = [&]( auto* node, int i, auto path ){
        if( node && node->children[ i ] ){
            auto* res = get_node( &result, path );
            if( !res->children[ i ] ){
                const auto* child = node->children[ i ].get();

                int id = child->id;
                while( seen.contains( id ) && i != 3 ){
                    ++id;
                }
                seen.insert( id );
                res->children[ i ] = std::make_shared< Tree >( id, child->side, child->in_connector, child->orientation, res );
                res->children[ i ]->children[ 3 ] = std::make_shared< Tree >( id, !child->side, 0, roficom::Orientation::North, res->children[ i ].get() );
                inserted += 1;
            }
            path.push_back( i );
            queue.emplace_back( node->children[ i ]->id, node->children[ i ]->side, path );
            path.push_back( 3 );
            queue.emplace_back( node->children[ i ]->id, !node->children[ i ]->side, path );
        }
    };

    while( !queue.empty() && inserted < world.modules().size() ){
        auto [ id, side, path ] = queue.front();
        queue.pop_front();

        auto* src = get_node( &source, path );
        auto* trg = get_node( &target, path );
        for( int i = 0; i < 3; i++ ){
            copy_node( src, i, path );
            copy_node( trg, i, path );
        }
    }

    return result;
}

std::vector< Tree* > TreeConfiguration::extra_leaves( Tree* node, Tree& target_tree ){
    std::vector< Tree* > reachable;
    for( auto* leaf : leaves ){
        // Tree* shoulder = get_shoulder( leaf );
        // double dist = distance( position( shoulder ), position( node ) );

        bool extra_node = get_node( &target_tree, get_mapping( get_path( leaf ) ) ) == nullptr;
        if( extra_node ){
            std::vector< int > partial;
            for( int i = 0; i < partial.size(); i++ ){
                auto* s_node = get_node( &tree, partial );
                auto* t_node = get_node( &target_tree, get_mapping( partial ) );
                partial.push_back( i );

                if( !match_connection( world, target_world, s_node, t_node, i, get_mapping( partial ).back() ) ){
                    extra_node = false;
                    break;
                }
            }
        }
        // --std::cout << "path: ";
        for( auto p : get_path( leaf ) ){
            // --std::cout << p << ' ';
        }
        // --std::cout << "extra: " << extra_node << '\n';
        if( extra_node ){
            reachable.push_back( leaf );
        }
    }
    std::ranges::sort( reachable, [&]( auto* l1, auto* l2 ){ 
        return distance( position( l1 ), position( node ) ) > distance( position( l2 ), position( node ));
    } );
    return reachable;
}

Tree* TreeConfiguration::closest_leaf( Tree* node, std::set< int > exclude ){
    Tree* min_leaf = nullptr;
    double dist = INFINITY;
    for( auto* leaf : leaves ){
        if( !exclude.contains( leaf->id ) && distance( position( leaf ), position( node ) ) < dist ){
            min_leaf = leaf;
        }
    }
    return min_leaf;
}

std::vector< RofiWorld > TreeConfiguration::fix_missing( Tree& target, Tree* s_node, Tree* t_node, int child, int mapped ){
    std::vector< RofiWorld > result;
    auto extra = extra_leaves( s_node, target );
    for( auto* leaf : extra ){
        if( leaf->id == s_node->id ){
            continue;
        }
        //// --std::cout << "connecting " << leaf->id << ", " << leaf->side << " with " << s_node->id << ", " << s_node->side << '\n';
            // << " via connectors " << source_connector << " and " << target_connector << '\n';
        //// --std::cout << "id: " << t_node->id << " conn: " << t_node->side * 3 + child << '\n';

        auto reconnected = reconnect_module( world, origin, s_node, leaf, child, t_node->children[ mapped ]->in_connector, t_node->children[ mapped ]->orientation );
        if( visited.contains( reconnected ) ){
            // --std::cout << "visited\n";
            continue;
        }
        if( std::ranges::count_if( s_node->children, []( const auto& child ){ return child != nullptr; } ) > 0
        ){
            // --std::cout << "reaching\n";
            result = connect_leaves( leaf, s_node, t_node->children[ mapped ]->in_connector, child, t_node->children[ mapped ]->orientation, false );
            if( result.empty() ){
                make_collision_tree();
            }
        } else {
            // --std::cout << "connecting\n";
            result = connect_leaves( leaf, s_node, t_node->children[ mapped ]->in_connector, child, t_node->children[ mapped ]->orientation, true );
        }
        if( !result.empty() ){
            Tree* to_disconnect = leaf->parent;
            // // --std::cout << "disconnecting: " << to_disconnect->id << ", " << to_disconnect->side << " conn " << to_disconnect->in_connector << " from " << to_disconnect->parent->id << '\n' << std::flush;
            disconnect( world, to_disconnect->id, to_disconnect->parent->id, to_disconnect->side * 3 + to_disconnect->in_connector );
            if( simulate ){
                auto rofi = rofis[ to_disconnect->id ];
                blocking_disconnect( rofi.getConnector( to_disconnect->side * 3 + to_disconnect->in_connector ) );
            }
            // --std::cout << "after disconnecting: " << serialization::toJSON( world ) << '\n';
            initialize_trees();
            return result;
        }
    }
    return result;
}

std::vector< RofiWorld > TreeConfiguration::fix_connection( Tree* s_node, int child, int connector, roficom::Orientation orientation, Tree& target ){

    auto s_id = s_node->id;
    auto s_side = s_node->side;
    auto t_id = s_node->children[ child ]->id;
    auto t_side = s_node->children[ child ]->side;
    auto s_in = s_node->children[ child ]->in_connector;
    std::vector< RofiWorld > result;

    result = remove_module( s_node->children[ child ].get(), target );
    if( !result.empty() ){
        s_node = get_node( &tree, s_id, s_side );
        auto* t = get_node( &tree, t_id, t_side );
        bool is_leaf = std::ranges::find( leaves, s_node ) != leaves.end();
        std::vector< RofiWorld > connect_result;
        if( s_in == connector && is_leaf ){
            connect_result = connect_leaves( s_node, t, child, connector, orientation, false );
        }
        if( !connect_result.empty() ){
            if( t->parent && t->parent->id == t->id ){
                t = t->parent;
            }
            disconnect( world, t, t->parent );
            if( simulate ){
                auto rofi = rofis[ t->id ];
                blocking_disconnect( rofi.getConnector( t->side * 3 + t->in_connector ) );
            }
            initialize_trees();
        }
    }
    return result;
}

bool TreeConfiguration::reconfigure_tree( Tree& target, [[maybe_unused]] const RofiWorld& target_world ){

    std::deque< std::vector< int > > queue;
    queue.emplace_back( std::vector< int >{} );
    std::map< std::vector< int >, std::string > states;
    std::map< std::vector< int >, int > stuck_count;
    std::vector< std::tuple< RofiWorld, int, double, std::map< std::vector< int >, std::vector< int > > > > fixes;
    std::vector< std::tuple< RofiWorld, int, double, std::map< std::vector< int >, std::vector< int > > > > fetches;

    while( !queue.empty() ){
        initialize_trees();

        auto path = queue.front();
        queue.pop_front();

        // --std::cout << "path from root: ";
        for( auto p : path ){
            // --std::cout << p << " ";
        }
        //// --std::cout << '\n';

        auto mapped_path = get_mapping( path );
        visited.insert( serialize( tree ) );

        auto* s_node = get_node( &tree, path );
        auto* t_node = get_node( &target, mapped_path );
        assert( s_node );
        assert( t_node );

        std::vector< RofiWorld > result;
        bool success = true;
        bool changed = false;

        for( int i = 0; i < 3; i++ ){
            RofiWorld copy = world;
            size_t conns = connections;
            double mov = movement;

            s_node = get_node( &tree, path );
            t_node = get_node( &target, mapped_path );
            
            std::vector< int > extended = path;
            extended.push_back( i );
            int mapped = get_mapping( extended ).back();


            if( !s_node->children[ i ] && t_node->children[ mapped ] ){
                // --std::cout << "missing branch\n";
                result = fix_missing( target, s_node, t_node, i, mapped );
                if( result.empty() ){
                    success = false;
                } else {
                    changed = true;
                }
                if( !result.empty() ){
                    fixes.emplace_back( copy, conns, mov, mapping );
                }
            }
            
            else if( s_node->children[ i ] && t_node->children[ mapped ] ){
                if( !match_connection( world, target_world, s_node, t_node, i, mapped ) )
                {
                    // --std::cout << "branch mismatch\n";
                    result = fix_connection( s_node, i, t_node->children[ mapped ]->in_connector, t_node->children[ mapped ]->orientation, target );
                    if( result.empty() ){
                        success = false;
                    } else {
                        success = false;
                        changed = true;
                        queue.push_front( path );
                    }
                }
            }
        }
        if( !success && !changed && stuck_count[ path ] > 2){
            // --std::cout << "fetching\n";
            RofiWorld copy = world;
            size_t conns = connections;
            double mov = movement;
            result = fetch_module( s_node, target );
            if( !result.empty() ){ 
                fetches.emplace_back( copy, conns, mov, mapping );
                queue.push_front( path );
            } else {
                success = false;
            }
        }


        if( states[ path ] == serialize( tree ) ){
            stuck_count[ path ] += 1;
        } else {
            stuck_count[ path ] = 0;
            states[ path ] = serialize( tree );
        }


        for( int i = 0; i < 4; i++ ){
            s_node = get_node( &tree, path );
            t_node = get_node( &target, get_mapping( path ) );
            path.push_back( i );
            int mapped = get_mapping( path ).back();
            if( s_node->children[ i ] && t_node->children[ mapped ] && ( i == 3 || 
                match_connection( world, target_world, s_node, t_node, i, mapped ) ) )
            {
                // --std::cout << "found\n";
                if( mapped < 2 && i < 2 && t_node->children[ mapped ]->in_connector != s_node->children[ i ]->in_connector ){
                    auto remapped = get_mapping( path );
                    auto p = path;
                    remapped.push_back( i == 0 ? 1 : 0 );
                    p.push_back( i );
                    mapping[ p ] = remapped;
                    remapped.pop_back();
                    p.pop_back();
                    remapped.push_back( i );
                    p.push_back( i == 0 ? 1 : 0 );
                    mapping[ p ] = remapped;
                }
                queue.push_back( path );
            }
            path.pop_back();
        }
        
        if( !success && get_node( &tree, path ) && std::ranges::find( queue, path ) == queue.end() ){
            queue.push_back( path );
        }
        else if( success ){
            set_node( target_world, target, path );
        }
        if( stuck_count[ path ] >= 10 && !simulate ){
            auto& backtracks = fetches.empty() ? fixes : fetches;
            if( backtracks.empty() ){
                return false;
            }
            world = std::get< 0 >( backtracks.back() );
            connections = std::get< 1 >( backtracks.back() );
            movement = std::get< 2 >( backtracks.back() );
            mapping = std::get< 3 >( backtracks.back() );
            initialize_trees();
            backtracks.pop_back();
            queue = { { } };
            stuck_count.clear();
        }
    }
    return true;
}

bool TreeConfiguration::set_node( const RofiWorld& target_world, Tree& target, const std::vector< int >& path, bool check_collisions ){
    RofiWorld copy = world;
    auto* node = get_node( &tree, path );
    auto* target_node = get_node( &target, get_mapping( path ) );
    assert( node );
    assert( target_node );

    auto* target_module = static_cast< UniversalModule* >( target_world.getModule( target_node->id ) );
    auto target_angle = target_node->side == 0 ? target_module->getAlpha() : target_module->getBeta();
    auto* node_module = static_cast< UniversalModule* >( copy.getModule( node->id ) );
    if( node->side == 0 ){
        node_module->setAlpha( target_angle );
    } else {
        node_module->setBeta( target_angle );
    }
    node_module->setGamma( target_module->getGamma() );
    copy.prepare().get_or_throw_as< std::logic_error >();
    if( !copy.isValid() && check_collisions ){
        return false;
    }
    if( simulate ){
        Manipulator dummy( world, false );
        dummy.arm.clear();

        dummy.arm.emplace_back( node->id, A );
        dummy.arm.emplace_back( node->id, B );

        simulate_movement( dummy, copy, true );
    }
    world = copy;
    return true;
}

std::vector< int > TreeConfiguration::get_mapping( const std::vector< int >& path ){
    std::vector< int > partial;
    std::vector< int > mapped;
    for( size_t i = 0; i < path.size(); i++ ){
        partial.push_back( path[ i ] );
        if( mapping.contains( partial ) ){
            mapped = mapping[ partial ];
        } else {
            mapped.push_back( path[ i ] );
        }
    }
    return mapped;
}

bool TreeConfiguration::reconfigure( const RofiWorld& target ){
    auto [ _, center ] = mcs_naive( world, target );
    TreeConfiguration t( target, center );

    target_world = t.world;
    bool result = reconfigure_tree( t.tree, t.world );
    if( result ){
        auto copy = TreeConfiguration( world, origin, false );
        copy.mapping = mapping;
        copy.tree.map( [&]( auto& node ){
            auto path = get_path( &node );
            copy.set_node( t.world, t.tree, path, false );
        });
        if( simulate ){
            Manipulator dummy( world, {}, &collision_tree, false );
            dummy.arm.clear();

            for( const auto& module : world.modules() ){
                dummy.arm.emplace_back( module.getId(), A );
                dummy.arm.emplace_back( module.getId(), B );

                simulate_movement( dummy, copy.world, true );
            }
        }
        world = copy.world;
        // --std::cout << "final: " << serialization::toJSON( world ) << '\n';
    }
    return result;
}

bool TreeConfiguration::reconfigure_snake(){
    bool snake = false;
    std::vector< RofiWorld > result;
    while( !snake ){
        //// --std::cout << "leaves: ";

        if( leaves.size() <= 2 ){
            connections += leaves.size();
            return true;
        }
        for( size_t i = 0; i < leaves.size(); i++ ){
            for( size_t j = 0; j < i; j++ ){
                result = connect_leaves( leaves[ i ], leaves[ j ], 2, 2, roficom::Orientation::North, true );
                if( !result.empty() ){
                    auto find_parent = [&]( Tree* node ){
                        while( node->parent ){
                            if( node->in_connector < 2 || node->orientation != roficom::Orientation::North ){
                                break;
                            }
                            node = node->parent;
                        }
                        return node;
                    };
                    Tree* to_disconnect = find_parent( leaves[ j ] );
                    if( to_disconnect == &tree ){
                        to_disconnect = find_parent( leaves[ i ] );
                    }
                    if( to_disconnect == &tree ){
                        for( int c = 0; c < 4; c++ ){
                            if( tree.children[ c ] ){
                                to_disconnect = tree.children[ c ].get();
                                break;
                            }
                        }
                        if( to_disconnect->id == tree.id ){
                            for( int c = 0; c < 4; c++ ){
                                if( tree.children[ c ] ){
                                    to_disconnect = tree.children[ c ].get();
                                    break;
                                }
                            }
                        }
                    }
                    //// --std::cout << "disconnecting: " << to_disconnect->id << ", " << to_disconnect->side << " conn " << to_disconnect->in_connector << " from " << to_disconnect->parent->id << '\n' << std::flush;
                    // getchar();
                    disconnect( world, to_disconnect->id, to_disconnect->parent->id, to_disconnect->side * 3 + to_disconnect->in_connector );
                    if( simulate ){
                        auto rofi = rofis[ to_disconnect->id ];
                        blocking_disconnect( rofi.getConnector( to_disconnect->side * 3 + to_disconnect->in_connector ) );
                    }
                    initialize_trees();
                    goto out;
                }
            }
        }
        out:
        snake = true;
        tree.map( [&]( const auto& node ){
            if( node != tree && ( node.in_connector < 2 || node.orientation != roficom::Orientation::North ) ){
                snake = false;
                //// --std::cout << "ruined on " << node.id << ", " << node.side << '\n';
            }
        });
    }
    //// --std::cout << "finished: " << serialization::toJSON( world ) << '\n';
    return true;
}

std::vector< reconfigurationStep > rofi::reconfiguration::reconfigure( const RofiWorld& source, const RofiWorld& target ){
    TreeConfiguration s( source );
    TreeConfiguration t( target );
    Tree intermediate = s.intermediate_tree( s.tree, t.tree );
    auto world = from_tree( t.tree );
    //// --std::cout << "target: " << serialization::toJSON( world ) << '\n';

    s.reconfigure_tree( t.tree, t.world );

    return {};
}

