#include "manipulator.hpp"

#include <catch2/catch.hpp>
#include <queue>
#include <stdexcept>
#include <variant>

using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace rofi::geometry;
using namespace rofi::kinematics;

Angle rofi::kinematics::get_rotation( const RofiWorld& world, int idx, int j ) {
    UniversalModule* ptr = static_cast<UniversalModule*>( world.getModule( idx ) );
    assert( ptr != nullptr );
    if( j == Alpha ){
        return ptr->getAlpha();
    }
    if( j == Beta ){
        return ptr->getBeta();
    }
    return ptr->getGamma();
}

void rofi::kinematics::copy_angles( const RofiWorld& target, RofiWorld& current ){
    for( const auto& mod : target.modules() ){
        if( mod.type != ModuleType::Universal ){
            continue;
        }
        const auto& um = static_cast< const UniversalModule& >( mod );
        auto* m = static_cast< UniversalModule* >( current.getModule( um.getId() ) );
        if( !m ){
            continue;
        }
        m->setAlpha( um.getAlpha() );
        m->setBeta( um.getBeta() );
        m->setGamma( um.getGamma() );
    }
    current.prepare().get_or_throw_as< std::logic_error >();
}

Manipulator::Manipulator( const RofiWorld& w, bool build_graph ) : world( w ), collision_tree( &_collision_tree ) {
    //initialize_config();
    //build_aabb();
    
    for( const auto& j : world.roficomConnections() ){
        joints.push_back(j);
    }
    for( const auto& [ module, pos ] : world.modulesWithAbsPos() ){
        if( module.type == ModuleType::Universal ){
            const auto& um = static_cast< const UniversalModule& >( module );
            arm.push_back( joint( module.getId(), A ) );
            arm.back().leaf = collision_tree->insert( Sphere( center( um.getBodyA().getPosition() ) ) );
            arm.back().leaf->rigid = false;
            arm.push_back( joint( module.getId(), B ) );
            arm.back().leaf = collision_tree->insert( Sphere( center( um.getBodyB().getPosition() ) ) );
            arm.back().leaf->rigid = false;
        }
        if( module.type == ModuleType::Cube ){
            collision_tree->insert( Sphere( center( pos ) ) );
        }
    }

    if( build_graph ){
        add_edges();
        generate_graph();
    }
}

Manipulator::Manipulator( const RofiWorld& w, const bodies& a, AABB< Sphere >* tree, bool build_graph ) : world( w ), arm( a ), collision_tree( tree )
{
    for( const auto& j : world.roficomConnections() ){
        joints.push_back(j);
    }
    if( build_graph ){
        for( auto& j : arm ){
            if( j.leaf )
                j.leaf->rigid = false;
        }
        generate_graph();
    }
}

/* Some backwards compatibility */
Edge roficom_edge( const RoficomJoint& roficom, const rofi::configuration::Module& src,
                   const rofi::configuration::Module& dest ){
    auto getConnector = []( auto conn ){
        switch( conn % 3 ){
            case 0:
                return XMinus;
            case 1:
                return XPlus;
            default:
                return ZMinus;
        }
    };
    auto getSide = []( auto conn ){
        return conn <= 3 ? A : B;
    };

    Edge edge = {
        src.getId(),
        getSide( roficom.sourceConnector ),
        getConnector( roficom.sourceConnector ),
        static_cast< Orientation >( roficom.orientation ),
        getConnector( roficom.destConnector ),
        getSide( roficom.destConnector ),
        dest.getId()
    };
    return edge;
}


void Manipulator::initialize_config(){
    Configuration config;
    joint root;
    Matrix center;
    for( const auto& module : world.modules() ){
        if( module.type == ModuleType::Universal ){
            const auto& mod = static_cast< const UniversalModule& >( module );
            config.addModule( mod.getAlpha().deg(),
                              mod.getBeta().deg(),
                              mod.getGamma().deg(),
                              mod.getId() );
        }
    }
    const auto& roficoms = world.roficomConnections();

    for( const auto& roficom : roficoms ){
        const auto& src = roficom.getSourceModule( world );
        const auto& dest = roficom.getDestModule( world );
        if( src.type == ModuleType::Pad && dest.type == ModuleType::Universal ){
            root.id = dest.getId();
            root.side = roficom.destConnector <= 3 ? A : B;
            auto component = roficom.destConnector <= 3 ? 7 : 8;
            center = src.components()[ component ].getPosition() * rotate( M_PI, { 0, 0, 1 } );
        } else if( dest.type == ModuleType::Pad && src.type == ModuleType::Universal ){
            root.id = src.getId();
            root.side = roficom.sourceConnector <= 3 ? A : B;
            auto component = roficom.sourceConnector <= 3 ? 7 : 8;
            center = src.components()[ component ].getPosition() * rotate( M_PI, { 0, 0, 1 } );
        }
        if( src.type == ModuleType::Universal && dest.type == ModuleType::Universal ){
            config.addEdge( roficom_edge( roficom, src, dest ) );
        }
    }

    tree = treeConfig( config, root, center );
    arm = tree.getFreeArm();
}


void Manipulator::build_aabb(){
    for( auto [ module, pos ] : world.modulesWithAbsPos() ){
        if( module.type == ModuleType::Cube ){
            tree.collisionTree.insert( Sphere( center( pos ) ) );
        }
        else if( module.type == ModuleType::Universal ){
            const auto& mod = static_cast< const UniversalModule& >( module );
            for( const auto& body : { mod.getBodyA(), mod.getBodyB() } ){
                auto* obj = tree.collisionTree.insert( Sphere( center( body.getPosition() ) ) );
                obj->rigid = false;
                for( auto& j : arm ){
                    if( j.id == mod.getId() &&
                        ( ( j.side == A && body == mod.getBodyA() ) ||
                          ( j.side == B && body == mod.getBodyB() ) ) )
                    {
                        j.leaf = obj;
                    }
                }
            }
        }
    }
}

void Manipulator::add_edges(){
    Matrix base = joint_position( 0 );
    double dist = double( arm.size() );
    for( auto dx : { -1.0, 0.0, 1.0 } ){
        for( auto dy : { -1.0, 0.0, 1.0 } ){
            for( auto dz : { 0.0, 1.0 } ){
                if( dx == 0 && dy == 0 && dz == 0 ){
                    continue;
                }
                Vector direction = { dx, dy, dz };
                direction /= magnitude( direction );
                direction *= dist;
                edges.push_back( base * direction );
            }
        }
    }
}

// If the line segment from a to b collides with no other objects, the spheres see each other
bool Manipulator::check_visibility( const Vector& a, const Vector& b ){
    return collision_tree->colliding_leaves( Segment( a, b ) ).size() <= 2;
}

void Manipulator::generate_graph(){
    std::vector< Sphere > obstacles;
    for( const auto& s : *collision_tree ){
        obstacles.push_back( s );
    }
    std::vector< Vector > vertices;
    std::vector< double > costs;
    for( size_t i = 0; i < obstacles.size(); i++ ){
        for( size_t j = 0; j < i; j++ ){
            bool visible = check_visibility( obstacles[ i ].center, obstacles[ j ].center );
            if( visible && distance( obstacles[ i ].center, obstacles[ j ].center ) >= 2.0 ){
                Vector point = midpoint( obstacles[ j ].center, obstacles[ i ].center );
                // Assign very high weight to points that we can barely fit between
                double cost = 100.0 / ( std::pow( distance( obstacles[ i ].center, obstacles[ j ].center ), 3 ) );
                //double cost = 1.0 / distance( obstacles[ i ].center, obstacles[ j ].center );
                vertices.emplace_back( point );
                costs.emplace_back( 1.0 );
            }
        }
        // for( const auto& edge : edges ){
        //     bool visible = check_visibility( obstacles[ i ].center, edge );
        //     if( visible ){
        //         Vector point = midpoint( obstacles[ i ].center, edge );
        //         // Assign very high weight to points that we can barely fit between
        //         double cost = 100.0 / ( std::pow( distance( obstacles[ i ].center, edge ), 3 ) );
        //         //double cost = 1.0 / distance( obstacles[ i ].center, obstacles[ j ].center );
        //         vertices.emplace_back( point );
        //         costs.emplace_back( cost );
        //     }
        // }
    }

    Matrix base = joint_position( 0 );
    for( double dx = -10.0; dx < 10.0; dx += 1.0 ){
        for( double dy = -10.0; dy < 10.0; dy += 1.0 ){
            for( double dz = 0.0; dz < 10.0; dz += 1.0 ){
                if( magnitude( Vector{ dx, dy, dz } ) < double( arm.size() + 1 ) && !(collision_tree->collides( Sphere( Vector{ dx, dy, dz, 1.0 }, 0.45 ) ) ) ){
                    vertices.emplace_back( base * Vector{ dx, dy, dz, 1.0 } );
                    costs.emplace_back( 1.0 );
                }
            }
        }
    }
    visibility_graph = Visibility( vertices, costs, *collision_tree );
}

std::vector< Matrix > Manipulator::generate_trajectory( const Matrix& source, const Matrix& target,
                                                        const std::vector< Vector >& path, double step_size )
{
    std::vector< Matrix > targets;
    Curve spline;
    spline.control_points = path;
    spline.order = 3;
    spline.knots.push_back( 0 );
    spline.knots.push_back( 0 );
    spline.knots.push_back( 0 );
    double d = 0.0;
    for( size_t i = 1; i < path.size(); i++ ){
        d += distance( path[ i ], path[ i - 1 ] );
        spline.knots.push_back( d );
    }
    spline.knots.push_back( spline.knots.back() );
    spline.knots.push_back( spline.knots.back() );

    auto s_angles = matrix_to_angles( source );
    auto t_angles = matrix_to_angles( target );

    double t = 0.1;
    while( t < spline.knots.back() ){
        t = std::min( t, spline.knots.back() );
        Vector p = spline.get_point( t );

        // If a sphere at the target would collide with any objects, it would
        // be unreachable; bend the trajectory to avoid that
        auto colliding = collision_tree->colliding_leaves( Sphere( p, 0.50 ) );
        int ctr = 0;
        while( !colliding.empty() ){
            if( ctr++ == 100 ){
                break;
            }
            for( const auto& s : colliding ){
                Vector diff = p - s.center;
                // //std::cout << "colliding:\n" << s.center;
                if( magnitude( diff ) < 1.0 ){
                    diff.at( 3 ) = 0;
                    diff /= magnitude( diff );
                    p = s.center + diff;
                }
                // //std::cout << "pos:\n" << p;
            }
            colliding = collision_tree->colliding_leaves( Sphere( p, 0.50 ) );
        }

        auto rotation = interpolate( s_angles, t_angles, std::min( t * t, spline.knots.back() )
                                     , spline.knots.back() );
        Matrix mat = translate( { p[ 0 ], p[ 1 ], p[ 2 ] } )
            * rotate( rotation[ 2 ], { 0, 0, 1 } )
            * rotate( rotation[ 1 ], { 0, 1, 0 } )
            * rotate( rotation[ 0 ], { 1, 0, 0 } );
        targets.push_back( mat );
        double step = step_size;
        // Determine size of next step based on whether there are any nearby objects
        // (constants completely ad-hoc based on testing on a few cases)
        colliding = collision_tree->colliding_leaves( Sphere( p, 1.0 ) );
        for( const auto& s : colliding ){
            Vector diff = p - s.center;
            step = std::min( ( magnitude( diff ) ) / 5.0, step );
        }
        t += std::min( step, spline.knots.back() / 5.0 );
        // t += 0.2;
    }

    return targets;
}

void normalize_configurations( const RofiWorld& initial, RofiWorld& target ){
    for( auto& m : target.modules() ){
        if( m.type != ModuleType::Universal ){
            continue;
        }
        auto& um = static_cast< UniversalModule& >( m );
        Angle previous = get_rotation( initial, m.getId(), Gamma );
        while( ( um.getGamma() - previous ).deg() > 180 ){
            um.setGamma( um.getGamma() - 360_deg );
        }
        while( ( um.getGamma() - previous ).deg() < -180 ){
            um.setGamma( um.getGamma() + 360_deg );
        }
    }
}

std::vector< double > Manipulator::relative_changes( const RofiWorld& target ) const {
    std::vector< double > diffs;
    Matrix end_position = static_cast< UniversalModule* >( world.getModule( arm.back().id ) )->getConnector( arm.back().side == A ? "A-X" : "B-X" ).getPosition();
    for( size_t i = 0; i < arm.size(); i++ ){
        auto x_joint = arm[ i ].side == A ? Alpha : Beta;
        Angle x = get_rotation( target, arm[ i ].id, x_joint ) - get_rotation( world, arm[ i ].id, x_joint );
        Angle z = i != 0 && arm[ i ].id == arm[ i - 1 ].id ? 
            get_rotation( target, arm[ i ].id, Gamma ) - get_rotation( world, arm[ i ].id, Gamma ) : Angle::rad( 0.0f );

        Matrix local = inverse( joint_position( i ) ) * end_position;
        double dist = distance( local, rotate( x.rad(), X ) * rotate( z.rad(), Z ) * local );
        diffs.push_back( 20.0 + dist );
    }
    return diffs;
}

std::vector< RofiWorld > Manipulator::generate_positions( const std::vector< Matrix >& trajectory ){
    std::vector< RofiWorld > positions = { world };

    size_t index = 0;
    int failcount = 0;
    for( const Matrix& m : trajectory ){
        bool last = ++index == trajectory.size();
        bool result = fabrik( m, last ? 0.05 : 0.3, last ? 0.05 : 1 );

        if( result ){
            positions.push_back( world );
            failcount = 0;
        } else {
            // positions.push_back( world );
            failcount++;
            if( last || failcount > 10 ){
               return {};
            }
        }
    }
    for( size_t i = 1; i < positions.size(); i++ ){
        normalize_configurations( positions[ i - 1 ], positions[ i ] );
    }
    return positions;
}

bool Manipulator::interpolate_positions( const RofiWorld& original, const RofiWorld& target, int speed ){
    auto diffs = relative_changes( target );
    auto max = *std::max_element( diffs.begin(), diffs.end() );
    
    bool finished = false;
    Manipulator copy( original, arm, collision_tree, false );
    // for( auto& body : copy.arm ){
    //     copy.collision_tree->erase( body.leaf );
    // }
    auto equals = []( float a, float b ){
        return std::fabs( a - b ) < 0.001;
    };

    auto sim_step = [&]( size_t idx, int j ){
        Angle target_rotation = get_rotation( target, arm[ idx ].id, j );
        Angle current_rotation = get_rotation( copy.world, arm[ idx ].id, j );
        // //std::cout << "current: " << current_rotation.deg() << " target: " << target_rotation.deg() << '\n';
        if( equals( target_rotation.deg(), current_rotation.deg() ) ){
            return current_rotation;
        } else {
            int sign = target_rotation.deg() > current_rotation.deg() ? 1 : -1;
            Angle angle = current_rotation + Angle::deg( float( speed * sign * diffs[ idx ] / max ) );
            if( ( target_rotation.deg() >= 0.0 && angle.deg() > target_rotation.deg() )
             || ( target_rotation.deg() < 0.0 && angle.deg() < target_rotation.deg() ) )
            {
                return target_rotation;
            }
            
            finished = false;
            // //std::cout << "target: " << target_rotation.deg() << " current: " << current_rotation.deg() << " angle: " << angle.deg() << '\n';
            return angle;
        }
    };
    while( !finished ){
        ////std::cout << iter++ << ' ';
        finished = true;
        ////std::cout << "step: " << serialization::toJSON( copy.world ) << '\n';
        for( size_t idx = 0; idx < arm.size(); idx++ ){
            int j = arm[ idx ].side == A ? Alpha : Beta;
            Angle x = sim_step( idx, j );
            // //std::cout << "finished: " << finished;
            Angle z = idx != 0 && arm[ idx ].id == arm[ idx - 1 ].id ? sim_step( idx, Gamma ) - get_rotation( copy.world, arm[ idx ].id, Gamma ) : 0_deg;
            copy.set_joint( idx, x, z );
        }
        auto result = copy.world.prepare();
        assert( result );
        for( size_t idx = 0; idx < arm.size(); idx++ ){
            if( !collision_tree->colliding_leaves( Sphere( center( copy.joint_position( idx ) ), 0.45 ) ).empty() ){
                return false;
            }
            for( size_t j = 0; j < idx; j++ ){
                if( collide( Sphere( center( copy.joint_position( idx ) ) ), Sphere( center( copy.joint_position( j ) ), 0.45 ) ) ){
                    return false;
                }
            }
        }
    }
    return true;
}

std::vector< RofiWorld > Manipulator::optimize( const std::vector< RofiWorld >& positions, const RofiWorld* disconnected ){
    if( positions.empty() ){
        return {};
    }
    std::vector< RofiWorld > optimized;
    if( disconnected ){
        optimized.push_back( *disconnected );
    } else {
        optimized.push_back( world );
    }

    for( auto& j : arm ){
        if( j.leaf ){
            // collision_tree->erase( j.leaf );
            // j.leaf = nullptr;
            j.leaf->rigid = false;
        }
    }
    size_t idx = 1;
    size_t last = 1;
    while( last != positions.size() - 1 )
    {
        idx = positions.size() - 1;
        RofiWorld target = positions[ idx ];
        if( disconnected ){
            target = *disconnected;
            copy_angles( positions[ idx ], target );
        }
        normalize_configurations( optimized.back(), target );
        while( idx > last && !interpolate_positions( optimized.back(), target, 2 ) ){
            target = positions[ --idx ];
            if( disconnected ){
                target = *disconnected;
                copy_angles( positions[ idx ], target );
            }
            normalize_configurations( optimized.back(), target );
        }
        if( idx == last ){
            // //std::cout << "positions:\n";
            for( const auto& p : positions ){
                copy_angles( p, target );
                // //std::cout << serialization::toJSON( target ) << '\n';
            }

            return {};
        }
        optimized.push_back( target );
        // //std::cout << serialization::toJSON( target ) << '\n';
        last = idx;
    }

    return optimized;
}

void Manipulator::add_endpoints( const Vector& src, const Vector& target ){
    if( visibility_graph.vertices.size() < visibility_graph.adjacency_matrix.n_rows ){
        visibility_graph.vertices.push_back( src );
        visibility_graph.vertices.push_back( target );
        visibility_graph.costs.push_back( 1.0 );
        visibility_graph.costs.push_back( 1.0 );
        visibility_graph.tax.push_back( 1.0 );
        visibility_graph.tax.push_back( 1.0 );
    } else {
        visibility_graph.vertices[ visibility_graph.vertices.size() - 2 ] = src;
        visibility_graph.vertices.back() = target;
    }
    for( size_t i = 0; i < visibility_graph.vertices.size(); i++ ){
        visibility_graph.connect_vertices( i, visibility_graph.vertices.size() - 2, tree.collisionTree );
        visibility_graph.connect_vertices( i, visibility_graph.vertices.size() - 1, tree.collisionTree );
    }
}

RoficomJoint missing_connection( const RofiWorld& current, const RofiWorld& disconnected ){
    for( const auto& connection : current.roficomConnections() ){
        if( std::find_if( disconnected.roficomConnections().begin(), disconnected.roficomConnections().end(),
        [&]( const auto& rj ){ 
            return ( rj.getSourceModule( disconnected ).getId() == connection.getSourceModule( current ).getId() 
                     && rj.getDestModule( disconnected ).getId() == connection.getDestModule( current ).getId() && 
                     rj.sourceConnector == connection.sourceConnector && rj.destConnector == connection.destConnector ) ||
                   ( rj.getSourceModule( disconnected ).getId() == connection.getDestModule( current ).getId() && 
                     rj.getDestModule( disconnected ).getId() == connection.getSourceModule( current ).getId() &&
                     rj.sourceConnector == connection.destConnector && rj.destConnector == connection.sourceConnector );
        } ) == disconnected.roficomConnections().end() ){
            return connection;
        }
    }
    throw std::exception();
    // assert( false );
}

std::vector< RofiWorld > Manipulator::reach( const Matrix& target, const RofiWorld* disconnected ){

    Matrix src = joint_position( arm.size() - 1 );
    add_endpoints( center( src ), center( target ) );
    RofiWorld copy = world;
    bool reachable = fabrik( target, 0.1, 0.1, false );
    if( !reachable ){
        // std::cout << "unreachable\n";
        return {};
    }
    normalize_configurations( copy, world );
    if( interpolate_positions( copy, world, 1 ) ){
        // std::cout << "First try: " << serialization::toJSON( world ) << '\n';
        // return { copy, world };
    } else {
        if( arm.size() == 2 ){
            return {};
        }
    }
    world = copy;

    for( size_t j = 1; j < arm.size(); ++j ){
        if( arm[ j ].leaf ){
            // std::cout << "erasing:\n" << center( joint_position( j ) );
            collision_tree->erase( Sphere( center( joint_position( j ) ) ) );
            arm[ j ].leaf = nullptr;
        }
    }

    if( !collision_tree->colliding_leaves( Sphere( center( target ), 0.45 ) ).empty() ){
        std::cout << "target:\n" << target << '\n';
        std::cout << "\noffedning leaves:\n";
        for( auto leaf : collision_tree->colliding_leaves( Sphere( center( target ), 0.5 ) ) ){
            std::cout << leaf.center;
        }
        return {};
    }
    // std::cout << "target:\n" << target << '\n';
    // for( auto leaf : *collision_tree ){
    //     std::cout << "leaf:\n" << leaf.center << '\n';
    // }

    std::vector< RofiWorld > positions;
    size_t paths = 0;
    RofiWorld original = world;
    while( positions.empty() || positions.size() == 1 ){
        paths++;
        auto path = visibility_graph.shortest_path( visibility_graph.vertices.size() - 2,
                                                    visibility_graph.vertices.size() - 1 );
        if( path.empty() ){
            //std::cout << "no path to target\n";
            return {};
        }
        // std::cout << "path:\n";
        for( auto p : path ){
            // std::cout << p << '\n';
        }

        auto trajectory = generate_trajectory( src, target, path );
        trajectory.push_back( target );

        positions = generate_positions( trajectory );
        world = original;
        if( positions.empty() ){
            paths++;
            // std::cout << "didn't reach\n";
        }
        // if( c == 'k' ){
        //     return positions;
        // }
        if( !positions.empty() && disconnected ){
            try{ 
                auto connection = missing_connection( world, *disconnected );
                // //std::cout << "src: " << connection.getSourceModule( world ).getId() << ", " << connection.sourceConnector 
                //     << " dest: " << connection.getDestModule( world ).getId() << ", " << connection.destConnector;
                RofiWorld dis = *disconnected;
                copy_angles( positions.back(), dis );
                dis.prepare().get_or_throw_as< std::logic_error >();
                if( !dis.isValid() ){
                    // positions.clear();
                    // std::cout << "invalid connection\n";
                }
                auto near = dis.getModule( connection.getSourceModule( world ).getId() )->components()[ connection.sourceConnector ].getNearConnector();
                if( !near ){
                    positions.clear();
                    // std::cout << "not precise enough to connect\n";
                } else {
                    // std::cout << "ok\n" << serialization::toJSON( dis ) << '\n';
                    // std::cout << "current:\n" << IO::toString( connection.getSourceModule( dis ).components()[ connection.sourceConnector ].getPosition() ) <<
                        // "near:\n" <<
                        // IO::toString( near.value().first.getPosition() );
                }
            } catch ( const std::exception& e ){
                positions.clear();
            }
        }

        // std::cout << "worlds:\n";
        // for( const auto& w : positions ){
        //     std::cout << serialization::toJSON( w ) << '\n';
        // }
        if( !positions.empty() )
            positions.back().prepare().get_or_throw_as< std::logic_error >();
        bool skip = !positions.empty() && positions.back().isValid(); // ugly ugly hack bcs I'm lazy
        RofiWorld w;
        if( !positions.empty() ) w = positions.back();
        positions = optimize( positions, disconnected );
        if( positions.empty() || positions.size() < 1 ){
            // std::cout << "interpolation failed\n";
        }
        if( positions.size() < 1 && fuckit ){
            normalize_configurations( original, w );
            return { w };
        }
        visibility_graph.raise_cost();
        if( positions.empty() ){
            if( paths >= arm.size() ){
                // for( size_t i = 0; i < arm.size(); ++i ){
                    // collision_tree->erase( arm[ i ].leaf );
                    // arm[ i ].leaf = collision_tree->insert( Sphere( center( joint_position( i ) ) ) );
                // }
                return {};
            }
        }

    }
    world = positions.back();

    return positions;
}

