#pragma once

#include "moduleState.hpp"
#include <map>

using namespace rofi::hal;

struct BotState
{
    std::map< Ip6Addr, ModuleState > modules;
    Ip6Addr moduleAddress;

    BotState( Ip6Addr moduleAddress ) : moduleAddress(moduleAddress) {}

    ModuleState currentModuleState()
    {
        return modules[ moduleAddress ];
    }
};