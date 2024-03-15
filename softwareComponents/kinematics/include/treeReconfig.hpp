#include <manipulator.hpp>
#include <rofi_hal.hpp>
#include "tree_utils.hpp"


namespace rofi::reconfiguration {

using namespace rofi::geometry;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace rofi::kinematics;
using namespace std::chrono_literals;

// void disconnect( RofiWorld& w, int src_id, int dest_id, int src_connector );

bool connected( const RofiWorld& world, int id, int connector );

RoficomJoint get_connection( const RofiWorld& world, int id, int connector );
//Tree make_tree( const RofiWorld& world, int center );
Tree mcs( const RofiWorld& source, const RofiWorld& target );

std::pair< int, int > mcs_naive( const RofiWorld& source, const RofiWorld& target );

RofiWorld from_tree( const Tree& tree, const Matrix& root = identity, bool ignore_first = false );

double difference( const RofiWorld& w1, const RofiWorld& w2 );

roficom::Orientation mirrored( roficom::Orientation ori );
bool symmetric( const RoficomJoint& joint, const RoficomJoint& other );

class TreeConfiguration {
  public:
    RofiWorld world;
    RofiWorld target_world;
    // RofiWorld target;
    Tree tree;
    AABB< Sphere > collision_tree;

    std::vector< Tree* > leaves;

    std::vector< hal::RoFI > rofis;

    bool simulate;

    size_t connections = 0;
    double movement;
    
    int max_length = 4;
    bool dodge = true;
    int origin = 0;

    std::map< std::vector< int >, std::vector< int > > mapping;

    std::set< std::string > visited;

  public:
    
    TreeConfiguration( const RofiWorld& w, int center = 0, bool sim = false, std::vector< hal::RoFI > rs = {}, int max_length = 4 );

    void make_collision_tree();
    void make_tree( const RofiWorld& world, int center, bool side = false );
    void initialize_trees();
    void add_redundant_connections( const RofiWorld& original );


  //private:
    Manipulator create_arm( Tree* shoulder );

    std::vector< RofiWorld > reach( Tree* leaf, Tree* target, int source_connector, int target_connector,
                                    roficom::Orientation orientation = roficom::Orientation::North, 
                                    const RofiWorld* disconnected = nullptr );

    std::vector< RofiWorld > approach_leaves( Tree* leaf, Tree* target, int source_connector, int target_connector,
                                        roficom::Orientation orientation );

    std::vector< RofiWorld > connect_leaves( Tree* leaf, Tree* target, int source_connector, int target_connector, 
                                                roficom::Orientation orientation, bool connect );

    std::vector< RofiWorld > fetch_module( Tree* target_node, Tree& target_tree );

    std::vector< RofiWorld > remove_module( Tree* target_node, Tree& target_tree );

    std::vector< RofiWorld > fix_connection( Tree* s_node, int child, int connector, roficom::Orientation orientation, Tree& target );

    std::vector< RofiWorld > reconnect_defect( Tree* defect );

    std::vector< RofiWorld > fix_missing( Tree& target, Tree* t_node, Tree* s_node, int child, int mapped );

    void simulate_movement( const Manipulator& manipulator, const RofiWorld& next, bool constant = false );
    bool connect();
    Tree intermediate_tree( Tree& source, Tree& target );
    bool reconfigure_tree( Tree& target, const RofiWorld& target_world );
    bool reconfigure_snake();
    bool reconfigure( const RofiWorld& target );

    bodies init_arm( Tree* leaf, Tree* base );
    Matrix position( const Tree* node ) const;
    Matrix shoe_position( const Tree* node ) const;
    // void disconnect( Tree* source, Tree* target );
    void straighten_branch( Tree* leaf );

    std::vector< Tree* > extra_leaves( Tree* node, Tree& target_tree );
    std::vector< Tree* > correct_leaves( Tree* node, Tree& target_tree );
    Tree* closest_leaf( Tree* node, std::set< int > exclude = {} );
    void reconnect_nodes( Tree* leaf, Tree* target, int source_connector, int target_connector, roficom::Orientation orientation, int count );

    void adjust_connectors( Tree* leaf, Tree* target, int source_connector, int target_connector );
    // Tree* closest_leaf( Tree* node, Tree& target );
    bool set_node( const RofiWorld& target_world, Tree& target, const std::vector< int >& path, bool check_collisions = true );
    void remove_movable( Tree* node );

    std::vector< int > get_mapping( const std::vector< int >& path );

};


std::vector< reconfigurationStep > reconfigure( const RofiWorld& source, const RofiWorld& target );

};