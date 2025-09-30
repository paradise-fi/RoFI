#pragma once

#include "moduleState.hpp"
#include <map>

using namespace rofi::hal;

struct BotState
{
    std::map< Ip6Addr, ModuleState > modules;
    Ip6Addr moduleAddress;

    BotState( Ip6Addr moduleAddress ) : moduleAddress(moduleAddress) {}

    ModuleState& currentModuleState()
    {
        return modules[ moduleAddress ];
    }

    void printBotState()
    {
        for (auto kvp = modules.begin(); kvp != modules.end(); ++kvp )
        {
            std::cout << "[==========================]" << std::endl;
            std::cout << kvp->second.moduleAddress << std::endl;
            for (auto connector = kvp->second.connectors.begin(); connector != kvp->second.connectors.end(); ++connector)
            {
                std::cout << "Connector " << connector->connectorId;
                if ( connector->connectedTo.has_value() )
                {
                    std::cout << " connected to module " << connector->connectedTo.value() << " = " << connector->otherSideConnectorId.value() << std::endl;
                }
                else
                {
                    std::cout << " not connected to any module." << std::endl;
                }
            }
        }
    }

    std::optional< std::pair< Ip6Addr, int > > tryFindLoop()
    {
        auto currentModule = currentModuleState();
        auto visitedModules = std::set< Ip6Addr >();
        return findLoopEnd( currentModule, currentModule.moduleAddress, -1, visitedModules );
    }

private:
    std::optional< std::pair< Ip6Addr, int > > findLoopEnd( ModuleState currentModule, Ip6Addr prevModule, int prevConnector, std::set< Ip6Addr >& visitedModules )
    {
        if ( visitedModules.find( currentModule.moduleAddress ) != visitedModules.end() )
        {
            return std::make_pair( prevModule, prevConnector );
        }

        visitedModules.emplace( currentModule.moduleAddress );

        for ( auto connector = currentModule.connectors.begin(); connector != currentModule.connectors.end(); ++connector )
        {
            if ( connector->otherSideConnectorId.has_value() && connector->otherSideConnectorId.value() == prevConnector )
            {
                continue;
            }

            if ( connector->connectedTo.has_value() )
            {
                auto result = findLoopEnd( modules[ connector->connectedTo.value() ], currentModule.moduleAddress, connector->connectorId, visitedModules );
                if ( result.has_value() )
                {
                    return result;
                }
            }
        }

        return std::nullopt;
    }
};