//
// Created by martin on 8/5/24.
//
#include "motor.hpp"

/**
     * Returns direction coeffiecient  -1 or 1 based on direction
     * @param position
     * @param goal_position
     * @return -1 if goal < position, 1 if goal > position, 0 if goal==position
 */
float getDirection(float position, float goal){
  float diff = position - goal;
  if( diff < 0.0f){
    return -1.0f;
  }
  if( diff > 0.0f){
    return 1.0f;
  }
  return 0.0f;
}

float getPower(float position, float goal_position){
  const float pTerm = 3.5; //proportial coefficent to distance to goal
  const float cTerm = 0.3; //minimum strength

  float positionFromGoal =  position - goal_position;
  float power = positionFromGoal * pTerm + getDirection(position, goal_position) * cTerm;

  return std::clamp( power, -1.0f, 1.0f);
}