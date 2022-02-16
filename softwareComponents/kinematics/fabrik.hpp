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
    Matrix trans = identity;

    std::array< Matrix, 2 > toNext = { { identity, identity } };
    std::array< int, 2 > nextIndex = { { 1, 1 } };
    std::array< bool, 2 > flip = { { false, false } };
    std::array< bool, 2 > xz = { { false, false } };
    std::array< bool, 2 > zx = { { false, false } };

    double xRot = 0.0;
    double zRot = 0.0;

    std::optional< Edge > nextEdge;
    std::optional< Edge > prevEdge;

    joint() = default;

    joint( ID id, ShoeId body ) : id( id ), body( body ){};

    Vector position(){
        return trans * Vector{ 0.0, 0.0, 0.0, 1.0 };
    }

    void setPosition( Vector position ){
        trans( 3, 0 ) = position[ 0 ];
        trans( 3, 1 ) = position[ 1 ];
        trans( 3, 2 ) = position[ 2 ];
    }

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

    bool connectArms( size_t arm1, size_t arm2 );

    bool reach( size_t arm, Vector position, double xRot = 0.0, double yRot = 0.0, double zRot = 0.0 );

    void printArm( size_t arm );

  //private:

    /* Find the arms of a rofibot */
    void findTentacles();
    void addJoints( ID id, Edge& edge, std::optional< Edge > lastEdge );

    void initialize();

    /* The core algorithm */
    bool fabrik( tentacle& arm, Matrix target );

    /* Simulate a connection between the arms */
    tentacle link( size_t arm1, size_t arm2 );

    /* Detach an arm from every other joint */
    void detach( size_t arm );

    void setArm( tentacle& arm, Matrix target );

    bool setJoint( ID id, Joint j, double rad );

};
