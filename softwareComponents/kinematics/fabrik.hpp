#include "kinematics.hpp"
#include <optional>
#include <stack>

enum joint_type {
    immovable,
    x_flexible,
    z_flexible,
    flexible
};

struct joint {
    ID id = 0;
    ShoeId body = A;
    // ConnectorId prevCon = ZMinus;
    // ConnectorId nextCon = ZMinus;
    //Vector position = { 0.0, 0.0, 0.0, 1.0 };
    Matrix trans = identity;
    double xRot = 0.0;
    double zRot = 0.0;
    // joint_type type = immovable;
    std::optional< Edge > nextEdge;
    std::optional< Edge > prevEdge;

    Vector position(){
        return trans * Vector{ 0.0, 0.0, 0.0, 1.0 };
    }

    void setPosition( Vector position ){
        trans( 3, 0 ) = position[ 0 ];
        trans( 3, 1 ) = position[ 1 ];
        trans( 3, 2 ) = position[ 2 ];
    }

    // inline Matrix get_matrix( int module, int shoe ){
    //     return config.getMatrices().at( module ).at( shoe );
    // }

    /* Global position of a joint, or a point converted from local
     * coordinates of the joint to global */
    inline Vector getGlobal( const Vector& local = { 0, 0, 0, 1 } ){
        return trans * local;
    }

    /* Global position to local */
    inline Vector getLocal( const Vector& global ){
        return inverse( trans ) * global;
    }

};



using tentacle = std::deque< joint >;

struct tentacleMonster {

//  private:

    Configuration config;

    ID leader = 0;

    std::vector< tentacle > tentacles;

    bool debug = false;

  public:

    tentacleMonster( Configuration c, int l = 0 ) : config( c ), leader( l ) {
        findTentacles();
    }

    tentacleMonster( std::string filename, int l = 0 ) : leader( l ) {
        std::ifstream input( filename );
        if( !input.is_open() ){
            throw std::exception();
        }
        Configuration conf;
        IO::readConfiguration( input, conf );
        *this = tentacleMonster( conf, l );
    }

    bool connectTentacle( int a, int b );

    bool reach( int arm, Vector position, double xRot = 0.0, double yRot = 0.0, double zRot = 0.0 );

    void printArm( int arm );

  private:

    void findTentacles();

    void addJoints( ID id, Edge edge, std::optional< Edge > lastEdge );

    bool fabrik( tentacle& arm, Matrix target );

    void setArm( tentacle& arm, Matrix target );

    bool setJoint( ID id, Joint j, double rad );

};
