#include "calculations.hpp"
#include <deque>
#include <cassert>
#include <math.h>

using chain = std::deque< int >;
using target = std::tuple< Vector, Vector, Vector, Vector >;

constexpr double error = 0.01;

enum class strategy { ccd, fabrik, pseudoinverse };

struct options {
    bool verbose = false;
    bool animate = false;
    bool random = false;
};

class kinematic_rofibot {

    // Configuration config;

    /* Is the configuration an arm fixed at it's lowest point, or something
     * with multiple arms? */
    bool fixed = false;

    /* Chain of IDs that belong to an arm */
    std::vector< chain > arms;

    options opt;

  public:
    Configuration config;

    kinematic_rofibot( Configuration new_config, bool fixed = false, options opt = {} );

    kinematic_rofibot( std::string file_name, bool fixed = false, options opt = {} );

    bool reach_random( strategy s = strategy::fabrik, int arm = 0 ){
        auto [ goal, x_frame, y_frame, z_frame ] = random_target( arms[ arm ] );
        
        if( s == strategy::ccd ){
            return ccd( goal, x_frame, y_frame, z_frame, arms[ arm ], 1000 );
        }
        if( s == strategy::fabrik ){
            return fabrik( goal, y_frame, z_frame, arms[ arm ], 100, arms[ arm ].size() );
        }
        if( s == strategy::pseudoinverse ){

            return pseudoinverse( goal, x_frame, z_frame, arms[ arm ] );
        }

        return false;
    }

    /* Reach a position with the given end-effector rotation */
    bool reach( Vector goal, std::vector< double > rotation = { 0, 0, 0 }, strategy s = strategy::fabrik, int arm = 0 ){
        int max_length = arms[ arm ].size() * 2 - 1;
        Vector y_frame = rotate( to_rad( rotation[ 0 ] ), X ) *
                            rotate( to_rad( rotation[ 1 ] ), Y ) *
                            rotate( to_rad( rotation[ 2 ] ), Z ) *
                            Vector( { 0, 1, 0, 1 } );
        Vector z_frame = rotate( to_rad( rotation[ 0 ] ), X ) *
                            rotate( to_rad( rotation[ 1 ] ), Y ) *
                            rotate( to_rad( rotation[ 2 ] ), Z ) *
                            Vector( { 0, 0, 1, 1 } );
        Vector x_frame = rotate( to_rad( rotation[ 0 ] ), X ) *
                            rotate( to_rad( rotation[ 1 ] ), Y ) *
                            rotate( to_rad( rotation[ 2 ] ), Z ) *
                            Vector( { 1, 0, 0, 1 } );

        if( distance( get_global( arms[ arm ].front(), 0 ), goal ) > max_length ){
            return false;
        }
        if( s == strategy::ccd ){
            return ccd( goal, x_frame, y_frame, z_frame, arms[ arm ], 1000 );
        }
        if( s == strategy::fabrik ){
            return fabrik( goal, y_frame, z_frame, arms[ arm ], 100, arms[ arm ].size() );
        }
        if( s == strategy::pseudoinverse ){

            return pseudoinverse( goal, x_frame, z_frame, arms[ arm ] );
        }

        return false;
    };

    /* Move the selected arms so that they can connect */
    bool connect( strategy s, int a = 0, int b = 1 ){
        if( a >= arms.size() || b >= arms.size() ){
            std::cerr << "Invalid arm index\n";
            return false;
        }
        if( distance( get_global( arms[ a ].front(), 0 ),
                      get_global( arms[ b ].front(), 0 ) ) > arms[ a ].size() + arms[ b ].size() )
        {
            std::cerr << "Impossible to connect\n";
            return false;
        }
        if( s == strategy::ccd ){
            return connect_ccd( a, b );
        }
        if( s == strategy::fabrik ){
            return connect_fabrik( a, b, 20 );
        }
        if( s == strategy::pseudoinverse ){
            return connect_pseudoinverse( a, b, 20 );
        }
        return false;
    };
    
    const inline Configuration& get_config(){
        return config;
    };

  private:

    /** Classic IK algorithm, Cyclic Coordinate Descent **/
    bool ccd( const Vector& goal, const Vector& x_frame, const Vector& y_frame,
              const Vector& z_frame, const chain& arm, int max_iterations = 100 );

    bool ccd2( const Vector& goal, const Vector& x_frame, const Vector& y_frame,
               const Vector& z_frame, const chain& arm, int max_iterations = 100 );

    bool connect_ccd( int a, int b, int max_iterations = 100 );

    /** Rotate a joint so that the end aligns with the goal **/
    bool rotate_to( const Vector& end_pos, const Vector& end_goal, int i, Joint joint,
                    bool check_validity = true, bool round = false );

    /** Rotate a joint if it results in a valid configuration **/
    bool rotate_if_valid( int module, Joint joint, double angle );

    /** Forward And Backward Reaching Inverse Kinematics (FABRIK) algorithm,
     ** adapted from "Aristidou A, Lasenby J.
     ** FABRIK: a fast, iterative solver for the Inverse kinematics problem"
     **/
    bool fabrik( const Vector& goal, const Vector& y_frame, const Vector& z_frame,
                 const chain& arm, int max_iterations = 100, int a_length = -1 );

    bool connect_fabrik( int a, int b, int max_iterations = 100 );

    /** Jacobian pseudoinverse IK algorithm **/
    bool pseudoinverse( const Vector& goal, const Vector& x_frame, const Vector& z_frame,
                        const chain& arm, int max_iterations = 10000 );

    bool connect_pseudoinverse( int a, int b, int max_iterations = 100 );

    arma::mat jacobian( const chain& arm );

    /* Generate a random configuration, and take its end-effector as target */
    target random_target( const chain& arm );

    /* Position of the last joint */
    inline Vector end_effector( const chain& arm ){
        return config.getMatrices().at( arm.back() ).at( 1 ) * Vector( { 0, 0, 0, 1 } );
    }

    /* Position after the last joint, i.e. to what Z can connect */
    inline Vector pointing_to( const chain& arm ){
        return config.getMatrices().at( arm.back() ).at( 1 ) * Vector( { 0, 0, -1, 1 } );
    }

    inline Matrix get_matrix( int module, int shoe ){
        return config.getMatrices().at( module ).at( shoe );
    }

    /* Global position of a joint, or a point converted from local
     * coordinates of the joint to global */
    inline Vector get_global( int module, int shoe, const Vector& local = { 0, 0, 0, 1 } ){
        return get_matrix( module, shoe ) * local;
    }

    /* Global position to local */
    inline Vector get_local( int module, int shoe, const Vector& global ){
        return inverse( get_matrix( module, shoe ) ) * global;
    }
};