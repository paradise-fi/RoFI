#include "calculations.hpp"
#include <deque>
#include <cassert>
#include <math.h>

using chain = std::deque< int >;

constexpr double error = 0.01;

enum class strategy { ccd, fabrik };

class rofi_bot {

    Configuration config;

    /* Position of the module with the lowest id; for now I always
     * consider it ( 0, 0, 0 ) */
    Vector center;

    /* Is the configuration an arm fixed at it's lowest point, or something
     * with multiple arms? */
    bool fixed = false;

    /* Chain of IDs that belong to an arm */
    std::vector< chain > arms;

  public:

    rofi_bot( Configuration new_config, bool fixed = false, Vector position = { 0, 0, 0, 1 } );

    rofi_bot( std::string file_name, bool fixed = false );

    /* Reach a position with the given end-effector rotation */
    template< strategy S = strategy::ccd >
    bool reach( Vector goal, std::vector< double > rotation = { 0, 0, 0 }, int arm = 0 ){
        int max_length = arms[ arm ].size() * 2 - 1;
        // TODO: might want to move as close as possible even if unreachable
        if( distance( get_global( arms[ arm ].front(), 0 ), goal ) > max_length ){
            return false;
        }
        if constexpr( S == strategy::ccd ){
            return ccd( goal, rotation, arms[ arm ] );
        }
        if constexpr( S == strategy::fabrik ){
            Vector pointing = goal + Vector( 
                        rotate( to_rad( rotation[ 0 ] ), X ) *
                        rotate( to_rad( rotation[ 1 ] ), Y ) *
                        rotate( to_rad( rotation[ 2 ] ), Z ) *
                        Vector( { 0, 0, 1, 1 } ) );
            return fabrik( goal, pointing, arms[ arm ] );
        }

        return false;
    };

    /* Move the selected arms so that they can connect */
    template< strategy S = strategy::ccd >
    bool connect( int a = 0, int b = 1 ){
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
        if constexpr( S == strategy::ccd ){
            return connect_ccd( a, b );
        }
        if constexpr( S == strategy::fabrik ){
            return connect_fabrik( a, b );
        }
        return false;
    };
    
    const inline Configuration& get_config(){
        return config;
    };

  private:

    bool connect_ccd( int a, int b, int max_iterations = 100 );

    /** Classic IK algorithm, Cyclic Coordinate Descent **/
    bool ccd( const Vector& goal, const std::vector< double >& rotation,
              const chain& arm, int max_iterations = 100 );

    bool rotate_to( const Vector& end_pos, const Vector& end_goal, int i, Joint joint,
                    bool check_validity = true, bool round = false );

    bool rotate_if_valid( int module, Joint joint, double angle );

    /** Forward And Backward Reaching Inverse Kinematics (FABRIK) algorithm,
     ** adapted from "Aristidou A, Lasenby J.
     ** FABRIK: a fast, iterative solver for the Inverse kinematics problem"
     **/
    bool fabrik( const Vector& goal, const Vector& pointing,
                 const chain& arm, int max_iterations = 100 );

    bool connect_fabrik( int a, int b, int max_iterations = 100 );

    /* Position of the last joint */
    inline Vector end_effector( const chain& arm ){
        return config.getMatrices().at( arm.back() ).at( 1 ) * Vector( { 0, 0, 0, 1 } );
    }

    /* Position after the last joint, i.e. to what Z can connect */
    inline Vector pointing_to( const chain& arm ){
        return config.getMatrices().at( arm.back() ).at( 1 ) * Vector( { 0, 0, -1, 1 } );
    }

    /* Global position of a joint, or a point converted from local
     * coordinates of the joint to global */
    inline Vector get_global( int module, int shoe, const Vector& local = { 0, 0, 0, 1 } ){
        return config.getMatrices().at( module ).at( shoe ) * local;
    }

    /* Global position to local */
    inline Vector get_local( int module, int shoe, const Vector& global ){
        return inverse( config.getMatrices().at( module ).at( shoe ) ) * global;
    }
};