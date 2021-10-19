#pragma once

#include <functional>
#include <map>
#include <optional>

#include "configuration/rofibot.hpp"
#include "inner_state.hpp"

#include <connectorCmd.pb.h>
#include <rofiDescription.pb.h>


namespace rofi::simplesim
{
class ModuleStates
{
public:
    using RofiDescription = rofi::messages::RofiDescription;
    using ConnectorState = ConnectorInnerState::ConnectorState;
    using ConnectorLine = rofi::messages::ConnectorCmd::Line;
    using ConnectedTo = std::optional< ConnectorInnerState::OtherConnector >;
    using JointVelocityControl = JointInnerState::VelocityControl;
    using JointPositionControl = JointInnerState::PositionControl;
    using JointControl = JointInnerState::Control;


    explicit ModuleStates( rofi::configuration::Rofibot && rofibotConfiguration )
            : _physicalModulesConfiguration( std::move( rofibotConfiguration ) )
            , _moduleInnerStates(
                      innerStatesFromConfiguration( this->_physicalModulesConfiguration ) )
    {}

    // Returns rofi description if module with moduleId exists
    std::optional< RofiDescription > getDescription( ModuleId moduleId ) const;


    // Returns velocity if module with moduleId exists and has given joint
    std::optional< JointControl > getJointControl( ModuleId moduleId, int joint ) const;

    // Sets position control and returns true if module with moduleId exists and has given joint
    bool setPositionControl( ModuleId moduleId, int joint, JointPositionControl positionControl );
    // Sets velocity control and returns true if module with moduleId exists and has given joint
    bool setVelocityControl( ModuleId moduleId, int joint, JointVelocityControl velocityControl );


    // Returns connector state if module with moduleId exists and has given connector
    std::optional< ConnectorState > getConnectorState( ModuleId moduleId, int connector ) const;
    // Returns the connectedTo connector if such exists
    std::optional< ConnectedTo > getConnectedTo( ModuleId moduleId, int connector ) const;

    // Extends connector and returns true if module with moduleId exists and has given connector
    bool extendConnector( ModuleId moduleId, int connector );
    // Retracts connector and returns true if module with moduleId exists and has given connector
    bool retractConnector( ModuleId moduleId, int connector );
    // Sets given power line and returns true if module with moduleId exists and has given connector
    bool setConnectorPower( ModuleId moduleId, int connector, ConnectorLine line, bool connect );

    std::set< ModuleId > getModuleIds() const
    {
        // Change to range in C++20
        auto moduleIds = std::set< ModuleId >();
        for ( auto & [ moduleId, _value ] : _moduleInnerStates ) {
            moduleIds.insert( moduleId );
        }
        return moduleIds;
    }


private:
    std::optional< std::reference_wrapper< const ModuleInnerState > > findModuleInnerState(
            ModuleId moduleId ) const
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return {};
        }

        return innerState->second;
    }
    std::optional< std::reference_wrapper< ModuleInnerState > > findModuleInnerState(
            ModuleId moduleId )
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return {};
        }

        return innerState->second;
    }

    std::optional< std::reference_wrapper< const ConnectorInnerState > > findConnectorInnerState(
            ModuleId moduleId,
            int connector ) const
    {
        auto moduleInnerState = findModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto connectorInnerStates = moduleInnerState->get().connectors();
        if ( connector < 0 || size_t( connector ) >= connectorInnerStates.size() ) {
            return {};
        }
        return connectorInnerStates[ connector ];
    }
    std::optional< std::reference_wrapper< ConnectorInnerState > > findConnectorInnerState(
            ModuleId moduleId,
            int connector )
    {
        auto moduleInnerState = findModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto connectorInnerStates = moduleInnerState->get().connectors();
        if ( connector < 0 || size_t( connector ) >= connectorInnerStates.size() ) {
            return {};
        }
        return connectorInnerStates[ connector ];
    }

    std::optional< std::reference_wrapper< const JointInnerState > > findJointInnerState(
            ModuleId moduleId,
            int joint ) const
    {
        auto moduleInnerState = findModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto jointInnerStates = moduleInnerState->get().joints();
        if ( joint < 0 || size_t( joint ) >= jointInnerStates.size() ) {
            return {};
        }
        return jointInnerStates[ joint ];
    }
    std::optional< std::reference_wrapper< JointInnerState > > findJointInnerState(
            ModuleId moduleId,
            int joint )
    {
        auto moduleInnerState = findModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto jointInnerStates = moduleInnerState->get().joints();
        if ( joint < 0 || size_t( joint ) >= jointInnerStates.size() ) {
            return {};
        }
        return jointInnerStates[ joint ];
    }

private:
    static std::map< ModuleId, ModuleInnerState > innerStatesFromConfiguration(
            const rofi::configuration::Rofibot & rofibotConfiguration );

private:
    rofi::configuration::Rofibot _physicalModulesConfiguration;
    std::map< ModuleId, ModuleInnerState > _moduleInnerStates;
};

} // namespace rofi::simplesim
