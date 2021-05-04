#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <iostream>

#include "rofi_hal.hpp"
using namespace rofi::hal;

// Hexapod contains 16 modules, with ids 1 - 17, see hexapod.in.
// Shape of the hexapod is as follows.
//
//             (head)
//
//             16A 17B
// 06A=06B-05A=05B-11A=11B-12A=12B 
//             15A=15B
// 04A=04B-03A=03B-09A=09B-10A=10B 
//             14A=14B
// 02A=02B-01A=01B-07A=07B-08A=08B 
//             13A=13B
//
//             (tail)
//
// Positive values of joints in 01A,03A,05A,07B,09B,11B go towards the head.
// Positive values of joints in 02B,04B,06B,08A,10A,12A go towards the ground.
// Shoes A contain alpha joints, shoes B contain beta joints.
// --------------------------------------------------------------------


std::vector< RoFI > modules;


// constants for joints, here we strictly assume universal RoFI modules
// --------------------------------------------------------------------
const int gam = 0; // center joint (gamma)
const int alf = 1; // joint in shoe A (alpha)
const int bet = 2; // joint in shoe B (beta)


// command and callback functions for monitoring the number of
// proceeding joint operations 
// --------------------------------------------------------------------
volatile int processing = 0;
constexpr float pi = 3.141592741f;
constexpr float fifteen = pi / 12;

void opFinished ( Joint ) {
  if ( processing > 0 )
    processing --;
  // std::cout << "-- op finished, remains "<< processing << std::endl;
}

void jointCommand( int module_id, int joint_id, float pos, int speed )
{
  processing ++;
  //std::cout << "# of running operations increased to " << processing << std::endl;
  std::cout << "Command: m="<<module_id<<" j="<<joint_id
	    << " speed=" << speed <<" pos=" << pos << std::endl;
  modules[module_id-1].getJoint(joint_id).setPosition( pos, speed, opFinished );
}


// main :-)
// --------------------------------------------------------------------

int main()
{

    for (int i = 1; i <= 17; i++) {
      modules.push_back( RoFI::getRemoteRoFI(i) );
      //      std::cout << RoFI::getRemoteRoFI(i).getId() <<std::endl;
    }

    const int speed = modules[0].getJoint(1).maxSpeed();   

    
    // 02A=02B-01A=01B-07A=07B-08A=08B 
    jointCommand( 2 , bet , -2*fifteen , speed ); // raise the leg off the ground
    while (processing >0) {};

    while (true)
    {


      // 02A=02B-01A=01B-07A=07B-08A=08B 
      
      jointCommand( 1 , alf , 2*fifteen , speed ); // move the leg forward from center
      jointCommand( 2 , bet , 0 , speed ); // lay the leg on the ground
      //jointCommand( 2 , bet , 3*fifteen , speed ); // lay the leg on the ground
      while (processing >0) {};

      jointCommand( 1 , alf , -2*fifteen , speed ); // move the leg backward
      while (processing >0) {};
      
      jointCommand( 2 , bet , -2*fifteen , speed ); // raise the leg off the ground
      jointCommand( 1 , alf , 0 , speed ); // center the leg 
      while (processing >0) {};
    }
        
    return 0;
}
