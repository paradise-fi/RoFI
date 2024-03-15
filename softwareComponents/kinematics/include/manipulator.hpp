#pragma once

#include <bitset>
#include <optional>
#include <queue>
#include <vector>

#include "calculations.hpp"
#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <fReconfig.hpp>

#include <atoms/units.hpp>
#include <geometry/aabb.hpp>
#include <geometry/curves.hpp>
#include <geometry/visibility.hpp>


#include <configuration/serialization.hpp>

namespace rofi::kinematics {

constexpr double eps = 0.001;

using namespace rofi::geometry;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;

using bodies = std::deque< joint >;

Angle get_rotation( const RofiWorld& world, int idx, int j );
void copy_angles( const RofiWorld& target, RofiWorld& current );

/* Wrapper around a RoFIbot that can move around objects in an environment with obstacles */
class Manipulator {
public:
    /* The full world with obstacles */
    RofiWorld world;

    treeConfig tree;

    bodies arm;

    std::vector< RoficomJoint > joints;

    AABB< Sphere >* collision_tree;

    AABB< Sphere > _collision_tree;

    Visibility visibility_graph;

    std::vector< Vector > edges;

    bool connecting = false;
    bool debug = false;

  public:

    Manipulator( const RofiWorld& w, bool build_graph = true );
    Manipulator( const RofiWorld& w, const bodies& a, AABB< Sphere >* tree, bool build_graph = true );
    //Manipulator( const RofiWorld& w, const std::vector< RoficomJoint >& joints );

    /* Reach specified target */
    std::vector< RofiWorld > reach( const Matrix& target, const RofiWorld* disconnected = nullptr );

    const RofiWorld& get_world(){
        return world;
    }
    const Configuration& get_config(){
        return tree.config;
    }
    const bodies& get_arm(){
        return arm;
    }

    Manipulator copy( const Matrix& target ) const;

    bool fabrik( Matrix target, double prec_position = 0.1, double prec_rotation = 0.1, bool check_collisions = true );

    void reaching( const Manipulator& other, bool check_collisions = false );

    void set_joint( size_t joint_index, Angle polar, Angle azimuth );
    
    std::pair< Angle, Angle > find_angles( size_t joint_index, const Manipulator& other ) const;

    std::pair< Angle, Angle > find_limit( size_t joint_index, const Vector& axis, bool x = true ) const;

    std::vector< Sphere > colliding_spheres( size_t joint_index, size_t body_index, const Vector& axis ) const;

    Matrix joint_position( size_t idx ) const {
        UniversalModule* ptr = static_cast<UniversalModule*>( world.getModule( arm[ idx ].id ) );
        assert( ptr != nullptr );
        if( idx == 0 || arm[ idx - 1 ].id != arm[ idx ].id ){
            return arm[ idx ].side == A ? ptr->components()[ 6 ].getPosition() : ptr->components()[ 9 ].getPosition();
        }
        return arm[ idx ].side == A ? ptr->getBodyA().getPosition() : ptr->getBodyB().getPosition();
    }

    Angle joint_rotation( size_t idx ) const {
        UniversalModule* ptr = static_cast<UniversalModule*>( world.getModule( arm[ idx ].id ) );
        assert( ptr != nullptr );

        return arm[ idx ].side == A ? ptr->getAlpha() : ptr->getBeta();
    }

    const RoficomJoint* get_connection( size_t in_idx, size_t out_idx ) const {
        for( const auto& connection : joints ){
            if( ( connection.getSourceModule( world ).getId() == arm[ in_idx ].id 
               && connection.getDestModule( world ).getId() == arm[ out_idx ].id )
             || ( connection.getSourceModule( world ).getId() == arm[ out_idx ].id 
               && connection.getDestModule( world ).getId() == arm[ in_idx ].id ) )
            {
                return &connection;
            }
        }
        return nullptr;
    }

    std::pair< Angle, Angle > find_end_angles( const Manipulator& other ) const;

    std::vector< double > relative_changes( const RofiWorld& target ) const;

  private:

    /* Initialize legacy configuration for the manipulator */
    void initialize_config();

    /* Initialize aabb tree from the configuration */
    void build_aabb();

    /* Add virtual obstacles to the edges of the workspace for vertex generation*/
    void add_edges();

    /* Check whether two spheres see each other */
    bool check_visibility( const Vector& a, const Vector& b );

    /* Generate visibility graph */
    void generate_graph();

    /* Add target and source to the visibility graph */
    void add_endpoints( const Vector& src, const Vector& target );

    /* Generate smooth trajectory from a path */
    std::vector< Matrix > generate_trajectory( const Matrix& source, const Matrix& target,
                                               const std::vector< Vector >& path, double step_size = 1.0 );

    /* Use fabrik to generate positions for given sequence of targets */
    std::vector< RofiWorld > generate_positions( const std::vector< Matrix >& trajectory );


    /* check whether a pair of configurations can be interpolated */
    bool interpolate_positions( const RofiWorld& original, const RofiWorld& target, int speed = 10 );
    

    /* Remove unnecessary targets from the sequence */
    std::vector< RofiWorld > optimize( const std::vector< RofiWorld >& positions, const RofiWorld* disconnected );

};

} // namespace rofi::kinematics
