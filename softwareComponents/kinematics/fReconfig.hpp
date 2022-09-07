#pragma once

//#include <calculations.hpp>
#include <deque>
#include <unordered_set>
#include <map>
#include <set>

#include <Configuration.h>
#include <IO.h>
#include "calculations.hpp"

/* Custom precision for considering matrices equal, more relaxed than Matrix.h/equals */
bool eq( const Matrix& a, const Matrix& b );
bool eq( const Vector& a, const Vector& b );

/* Simple representation for arms consisting of joints */
struct joint {
    ID id;
    ShoeId side;

    bool operator==( joint b ) const {
        return id == b.id && side == b.side;
    }

    bool operator<( joint b ) const {
        return id < b.id || ( side == A && b.side == B );
    }
};

using joints = std::deque< joint >;

/* Representation for actions in the final reconfiguration */
enum class actionType {
    rotation, connection, disconnect
};

struct reconfigurationStep {
    actionType type = actionType::rotation;
    int id = 0;
    Joint j = Alpha;
    double angle = 0.0;

    Edge edge = { 0, A, ZMinus, 0, ZMinus, A, 0 };

};

reconfigurationStep setRotation( ID id, Joint j, double angle );
reconfigurationStep setConnection( Edge toConnect );
reconfigurationStep setDisconnect( Edge toDisconnect );

enum class collisionStrategy {
    all, none, naive, online
};

enum class straightening {
    all, none, onCollision, always
};

std::string toString( collisionStrategy coll );
std::string toString( straightening str );

/* Reconfiguration machine */
struct treeConfig {

    /* Configuration and it's center */
    Configuration config;
    ID root;

    std::map< ID, int > depths;

    /* Flags for reconfiguration */
    collisionStrategy collisions;
    straightening straight;

    /* Logging of steps taken to reconfigure */
    std::vector< reconfigurationStep > reconfigurationSteps;
    std::vector< reconfigurationStep > waitingConnections;
    std::vector< reconfigurationStep > waitingDisconnects;

    /* Initialize and treefy configuration */
    treeConfig( const std::string& path );
    treeConfig( const std::string& path, ID r );
    treeConfig( Configuration c );
    treeConfig( Configuration c, ID r );

    /* Initialize arm for single arm configurations */
    joints getFreeArm();
    joints initArm( ID id, ShoeId side, std::unordered_set< ID >& seen );

    /* Get all arms for reconfiguration */
    std::vector< joints > getFreeArms();

    /* Find connected modules */
    int connections( ID id, ShoeId side, ID exclude );
    joint getConnected( ID id, ShoeId side, std::unordered_set< ID > exclude );

    /* Simple reconfiguration algorithm - try to connect all arms, backtrack through recursion */
    bool tryConnections();

    /* If a single arm is left but the connections are wrong, fix them up */
    bool fixConnections();

    /* Connect arms by linking them and using FABRIK */
    bool connect( joints arm1, joints arm2, bool straighten = true );

    /* Link the two arms, return a copy of the old configuration in case of failure */
    Configuration link( joints& arm1, joints& arm2 );

    /* Fabrik itself, takes arm of the configuration and tries to reach target */
    bool fabrik( const joints& arm, const Matrix& target );

    /* Helper functions for FABRIK */
    Configuration initBackArm( const joints& arm, const Matrix& target );
    void reaching( const joints& arm, Configuration& backArm, const Matrix& target );

    Edge edgeBetween( const joint& j1, const joint& j2 );

    Matrix nextPosition( const joints& arm, size_t current, Configuration& currentConfig,
                         bool forward );

    std::pair< double, double > computeAngles( const joints& arm, size_t currentJoint,
                                               Configuration& currentConfig,
                                               Configuration& otherConfig,
                                               Matrix currentPos, Matrix otherPos,
                                               Matrix nextPos, bool forward );

    std::pair< double, double > computeJoints( const joints& arm, size_t currentJoint,
                                               Configuration& currentConfig,
                                               Configuration& otherConfig,
                                               bool forward );
    void rotateJoints( const joints& arm, size_t currentJoint,
                       Configuration& currentConfig, double polar, double azimuth );

    void rotateTowards( const joints& arm, size_t currentJoint, const Matrix& target );

    void straightenArm( const joints& arm );

    joint getPrevious( joint j );

    void extend( joints& arm1, joints& arm2 );

    bool goodConnections( const joints& arm );

    std::set< joint > collidingJoints( joint j );
    std::pair< Vector, double > intersectionCircle( joint current, joint colliding );
    void fixCollisions( const joints& arm, size_t currentJoint );
    /* Option for debugging step by step */
    bool debug = false;
};

void toVideo( std::string outputFile, std::string fromConfig, std::string toConfig );
