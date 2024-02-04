#pragma once

#include <cassert>
#include <future>
#include <iostream>

#include <atoms/units.hpp>

#include "rofi_hal.hpp"

using namespace rofi::hal;

namespace rofi::macro {

template < typename Callback >
void setJointPos( Joint joint, float pos, Callback && callback = nullptr, float speedMultiplier = 0.8f )
{
    auto speed = joint.maxSpeed() * speedMultiplier;

    joint.setPosition(
            pos,
            speed,
            [ callback = std::forward< Callback >( callback ) ]( Joint ) { callback(); } );
}

void setJointPosWait( Joint joint, float pos, float speedMultiplier = 0.8f );

void extend( Connector connector );

void retract( Connector connector );

bool tryConnect( Connector connector );

void resetJointsWait( RoFI& rofiMod );

bool moveWait( RoFI& rofiMod, int bottomConnId, bool forward = true );

} // namespace rofi::macro
