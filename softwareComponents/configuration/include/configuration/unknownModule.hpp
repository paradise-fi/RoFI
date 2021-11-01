#pragma once

#include <array>
#include <iostream>
#include <configuration/rofibot.hpp>

namespace rofi::configuration {

class UnknownModule : public Module {

public:
    UnknownModule( std::vector< Component > components,
        int connectorCount, std::vector< ComponentJoint > joints, ModuleId id,
        std::optional< int > rootComponent = std::nullopt)
    : Module( ModuleType::Unknown, components, connectorCount, joints, id, rootComponent ) {}

    ATOMS_CLONEABLE( UnknownModule );
};

} // namespace rofi::configuration
