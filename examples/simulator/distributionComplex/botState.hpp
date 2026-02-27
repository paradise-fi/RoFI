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

    std::vector< std::pair< Ip6Addr, int > > findLongestStubArm()
    {
        auto currentModule = currentModuleState();
        auto visitedModules = std::set< Ip6Addr >();
        
        std::map< Ip6Addr, Ip6Addr > prevModules;

        std::vector< std::pair< Ip6Addr, int > > stubModules;
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
                        prevModules.emplace( connector->connectedTo.value(), moduleAddr );
                    }
                    visited.emplace( moduleAddr );
                    connectedCount++;
                    latestConnectorId = connector->connectorId;
                }
            }

            if ( connectedCount == 1 )
            {
                // 0 - 2 is joint 0, 3 - 5 joint 1
                stubModules.push_back( std::make_pair< Ip6Addr, int >( std::move( moduleAddr ), latestConnectorId / 3 ) );
            }
        }

        if (stubModules.empty())
        {
            return std::vector< std::pair< Ip6Addr, int > >();
        }

        std::vector< std::pair< Ip6Addr, int > > longestStub;
        
        size_t longestLength = 0;
        
        for ( auto stub = stubModules.begin(); stub != stubModules.end(); ++stub )
        {
            auto result = resolveStubLimb( prevModules, *stub );
            if ( result.size() > longestLength )
            {
                longestStub = result;
                longestLength = result.size();
            }
        }
        
        return longestStub;
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
    std::vector< std::pair< Ip6Addr, int > > resolveStubLimb( std::map< Ip6Addr, Ip6Addr >& previousModules, std::pair< Ip6Addr, int > module )
    {
        std::vector< std::pair< Ip6Addr, int > > result;
        result.push_back( module );
        auto currentModule = previousModules.find( module.first );
        while ( currentModule != previousModules.end() )
        {
            result.push_back( std::make_pair( currentModule->second, 0 ) );
            currentModule = previousModules.find( currentModule->second );
        }
        return result;
    }

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