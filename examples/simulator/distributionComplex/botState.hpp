#pragma once

#include "moduleState.hpp"
#include <map>
#include <queue>
#include <set>

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

    std::optional< std::pair< Ip6Addr, int > > findConnectedStubJoint()
    {
        auto currentModule = currentModuleState();
        auto visitedModules = std::set< Ip6Addr >();
        
        std::set< Ip6Addr > visited;
        std::queue< Ip6Addr > moduleQueue;
        moduleQueue.push( currentModule.moduleAddress );
        visited.emplace( currentModule.moduleAddress );

        while ( !moduleQueue.empty() )
        {
            Ip6Addr moduleAddr = moduleQueue.front();
            moduleQueue.pop();

            auto modState = modules.find( moduleAddr );
            if ( modState == modules.end() )
            {
                continue;
            }

            auto& module = modState->second;

            int connectedCount = 0;
            int latestConnectorId = 0;
            for ( auto connector = module.connectors.begin(); connector != module.connectors.end(); ++connector )
            {
                if ( connector->connectedTo.has_value() )
                {
                    if ( visited.find( connector->connectedTo.value() ) == visited.end() )
                    {
                        moduleQueue.push( connector->connectedTo.value() );
                    }
                    visited.emplace( moduleAddr );
                    connectedCount++;
                    latestConnectorId = connector->connectorId;
                }
            }

            if ( connectedCount == 1 )
            {
                // 0 - 2 is joint 0, 3 - 5 joint 1
                return std::make_pair< Ip6Addr, int >( std::move( moduleAddr ), latestConnectorId / 3 );
            }
        }

        return std::nullopt;
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