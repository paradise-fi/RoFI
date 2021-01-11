#include "calculations.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

int main( int argc, char** argv ){
    kinematic_chain kc;

    if( argc < 3 ){
        std::cerr << "Usage: ./ccd 'path to initial config' 'x y z α β γ'" << std::endl;
        return 1;
    }

    std::ifstream input( argv[ 1 ] );
    if( !input.is_open() ){
        std::cerr << "Couldn't open file \'" << argv[ 1 ] << "\'" << std::endl;
        return 1;
    }

    IO::readConfiguration( input, kc.config );

    input.close();

    std::stringstream ss( argv[ 2 ] );
    std::vector< double > goal = { 0, 0, 0, 1 };
    ss >> goal[ 0 ] >> goal[ 1 ] >> goal[ 2 ];

    kc.goals.push_back( goal );

    double alpha, beta, gamma;
    ss >> alpha >> beta >> gamma;
    std::vector< double > helper_goal = goal + rot_matrix( to_rad( alpha ), to_rad( beta ), to_rad( gamma ) ) * std::vector{ 0.0, 0.0, 1.0 };

    std::cerr << "helper goal: " << helper_goal;
    kc.goals.push_back( helper_goal );

    kc.init();

    kc.reach( kc.joint_positions.size() - 2, 0 );
    kc.reach( kc.joint_positions.size() - 1, 1 );
    kc.reach( kc.joint_positions.size() - 2, 0, true );

    size_t i = kc.chain.size() - 2;

    std::vector< double > local_end = kc.joint_positions.back();
    std::vector< double > local_goal = helper_goal;

    local_end = kc.joint_positions.back();
    local_goal = helper_goal;
    for( int j = 0; j < kc.chain.size() - 1; ++j ){
        local_end = kc.chain[ j ].from_previous() * local_end;
        local_goal = kc.chain[ j ].from_previous() * local_goal;
    }
    kc.chain[ i ].rz = kc.chain[ i ].rz + rotz( local_end, local_goal );
    kc.set_position( i );

    local_end = kc.joint_positions.back();
    local_goal = helper_goal;
    for( int j = 0; j < kc.chain.size() - 1; ++j ){
        local_end = kc.chain[ j ].from_previous() * local_end;
        local_goal = kc.chain[ j ].from_previous() * local_goal;
    }
    kc.chain.back().rx = std::clamp( kc.chain.back().rx + rotx( local_end, local_goal ), -M_PI / 2, M_PI / 2 );
    kc.set_position( i );


    for( auto& vec : kc.joint_positions ){
        std::cerr << vec;
    }
    size_t current = 1;
    for( int id : kc.ids ){
        kc.config.getModule( id ).setJoint( Joint::Alpha, to_deg( kc.chain[ current ].rx ) );
        kc.config.getModule( id ).setJoint( Joint::Beta, to_deg( kc.chain[ current + 1 ].rx ) );
        kc.config.getModule( id ).setJoint( Joint::Gamma, to_deg( kc.chain[ current ].rz ) );
        current += 2;
    }

    std::cout << IO::toString( kc.config );
    
}
