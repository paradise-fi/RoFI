#include "module_states.hpp"


using namespace rofi::configuration;
using namespace rofi::simplesim;


std::optional< ModuleStates::RofiDescription > ModuleStates::getDescription(
        ModuleId moduleId ) const
{
    auto moduleInnerState = findModuleInnerState( moduleId );
    if ( !moduleInnerState ) {
        return {};
    }

    auto description = std::make_optional< RofiDescription >();
    description->set_jointcount( moduleInnerState->get().joints().size() );
    description->set_connectorcount( moduleInnerState->get().connectors().size() );
    return description;
}

std::optional< ModuleStates::JointControl > ModuleStates::getJointControl( ModuleId moduleId,
                                                                           int joint ) const
{
    auto jointInnerState = findJointInnerState( moduleId, joint );
    if ( !jointInnerState ) {
        return {};
    }

    return jointInnerState->get().control();
}

bool ModuleStates::setPositionControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointPositionControl positionControl )
{
    auto jointInnerState = findJointInnerState( moduleId, joint );
    if ( !jointInnerState ) {
        return false;
    }

    jointInnerState->get().setPositionControl( std::move( positionControl ) );
    return true;
}

bool ModuleStates::setVelocityControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointVelocityControl velocityControl )
{
    auto jointInnerState = findJointInnerState( moduleId, joint );
    if ( !jointInnerState ) {
        return false;
    }

    jointInnerState->get().setVelocityControl( std::move( velocityControl ) );
    return true;
}

std::optional< ModuleStates::ConnectorState > ModuleStates::getConnectorState( ModuleId moduleId,
                                                                               int connector ) const
{
    auto connectorInnerState = findConnectorInnerState( moduleId, connector );
    if ( !connectorInnerState ) {
        return {};
    }

    return connectorInnerState->get().connectorState();
}

std::optional< ModuleStates::ConnectedTo > ModuleStates::getConnectedTo( ModuleId moduleId,
                                                                         int connector ) const
{
    auto connectorInnerState = findConnectorInnerState( moduleId, connector );
    if ( !connectorInnerState ) {
        return {};
    }

    return connectorInnerState->get().connectedTo();
}

bool ModuleStates::extendConnector( ModuleId moduleId, int connector )
{
    auto connectorInnerState = findConnectorInnerState( moduleId, connector );
    if ( !connectorInnerState ) {
        return false;
    }

    connectorInnerState->get().extend();
    return true;
}

bool ModuleStates::retractConnector( ModuleId moduleId, int connector )
{
    auto connectorInnerState = findConnectorInnerState( moduleId, connector );
    if ( !connectorInnerState ) {
        return false;
    }

    connectorInnerState->get().retract();
    // TODO disconnect this and the connectedTo connector
    return true;
}

bool ModuleStates::setConnectorPower( ModuleId moduleId,
                                      int connector,
                                      ConnectorLine line,
                                      bool connect )
{
    using ConnectorCmd = rofi::messages::ConnectorCmd;

    auto connectorInnerState = findConnectorInnerState( moduleId, connector );
    if ( !connectorInnerState ) {
        return false;
    }

    switch ( line ) {
        case ConnectorCmd::INT_LINE:
        {
            connectorInnerState->get().internal() = connect;
            return true;
        }
        case ConnectorCmd::EXT_LINE:
        {
            connectorInnerState->get().external() = connect;
            return true;
        }
        default:
        {
            return false;
        }
    }
}

std::map< ModuleId, ModuleInnerState > ModuleStates::innerStatesFromConfiguration(
        const Rofibot & rofibotConfiguration )
{
    auto innerStates = std::map< ModuleId, ModuleInnerState >();
    for ( const auto & moduleInfo : rofibotConfiguration.modules() ) {
        auto & _module = *moduleInfo.module.get();

        constexpr auto jointCount = 3; // TODO get from the configuration
        auto moduleInnerState = ModuleInnerState( jointCount, _module.connectors().size() );

        auto [ it, success ] = innerStates.emplace( _module.getId(),
                                                    std::move( moduleInnerState ) );
        if ( !success ) {
            throw std::runtime_error( "Multiple same module ids in configuration" );
        }
    }

    return innerStates;
}
