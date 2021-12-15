#include "simplesim/module_states.hpp"


using namespace rofi::configuration;
using namespace rofi::simplesim;


std::optional< ModuleStates::RofiDescription > ModuleStates::getDescription(
        ModuleId moduleId ) const
{
    if ( auto moduleInnerState = getModuleInnerState( moduleId ) ) {
        RofiDescription description;
        description.set_jointcount( moduleInnerState->get().joints().size() );
        description.set_connectorcount( moduleInnerState->get().connectors().size() );
        return description;
    }
    return {};
}

// Returns joint position limits if module with moduleId exists and has given joint
std::optional< std::pair< float, float > > ModuleStates::getJointPositionLimits( ModuleId moduleId,
                                                                                 int joint ) const
{
    auto currentConfig = currentConfiguration();
    assert( currentConfig );

    if ( auto modulePtr = currentConfig->getModule( moduleId ) ) {
        const auto & joints = modulePtr->joints();
        if ( joint >= 0 && static_cast< size_t >( joint ) < joints.size() ) {
            const auto & jointLimits = joints[ joint ].joint->jointLimits();
            if ( !jointLimits.empty() ) {
                assert( jointLimits.size() == 1 );
                return jointLimits.front();
            }
        }
    }
    return {};
}

std::optional< ModuleStates::JointCapabilities > ModuleStates::getJointCapabilities(
        ModuleId moduleId,
        int joint ) const
{
    if ( auto jointLimits = getJointPositionLimits( moduleId, joint ) ) {
        JointCapabilities capabilities;
        capabilities.set_minposition( jointLimits->first );
        capabilities.set_maxposition( jointLimits->second );

        // TODO set max speed on per module/per joint bases
        capabilities.set_maxspeed( ModuleStates::maxJointSpeed );

        return capabilities;
    }
    return {};
}

std::optional< float > ModuleStates::getJointPosition( ModuleId moduleId, int joint ) const
{
    auto currentConfig = currentConfiguration();
    assert( currentConfig );

    if ( auto modulePtr = currentConfig->getModule( moduleId ) ) {
        const auto & joints = modulePtr->joints();
        if ( joint >= 0 && static_cast< size_t >( joint ) < joints.size() ) {
            const auto & positions = joints[ joint ].joint->positions();
            if ( !positions.empty() ) {
                assert( positions.size() == 1 );
                return positions.front();
            }
        }
    }
    return {};
}

std::optional< float > ModuleStates::getJointVelocity( ModuleId moduleId, int joint ) const
{
    auto jointInnerState = getJointInnerState( moduleId, joint );
    if ( jointInnerState ) {
        return jointInnerState->get().velocity();
    }
    return {};
}

bool ModuleStates::setPositionControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointPositionControl positionControl )
{
    if ( auto jointInnerState = getJointInnerState( moduleId, joint ) ) {
        jointInnerState->get().setPositionControl( std::move( positionControl ) );
        return true;
    }
    return false;
}

bool ModuleStates::setVelocityControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointVelocityControl velocityControl )
{
    if ( auto jointInnerState = getJointInnerState( moduleId, joint ) ) {
        jointInnerState->get().setVelocityControl( std::move( velocityControl ) );
        return true;
    }
    return false;
}

std::optional< ModuleStates::ConnectorState > ModuleStates::getConnectorState( ModuleId moduleId,
                                                                               int connector ) const
{
    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        return connectorInnerState->get().connectorState();
    }
    return {};
}

std::optional< ModuleStates::ConnectedTo > ModuleStates::getConnectedTo( ModuleId moduleId,
                                                                         int connector ) const
{
    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        return connectorInnerState->get().connectedTo();
    }
    return {};
}

bool ModuleStates::extendConnector( ModuleId moduleId, int connector )
{
    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        connectorInnerState->get().setExtended();
        // TODO check if there is a near connector
        return true;
    }
    return false;
}

std::optional< ModuleStates::ConnectedTo > ModuleStates::retractConnector( ModuleId moduleId,
                                                                           int connector )
{
    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        auto connectedTo = connectorInnerState->get().resetConnectedTo();
        connectorInnerState->get().setRetracted();

        if ( connectedTo ) {
            if ( auto otherConnInner = getConnectorInnerState( connectedTo->moduleId,
                                                               connectedTo->connector ) ) {
                [[maybe_unused]] auto otherConnectedTo = otherConnInner->get().resetConnectedTo();

                assert( otherConnectedTo );
                assert( otherConnectedTo->moduleId == moduleId );
                assert( otherConnectedTo->connector == connector );
                assert( otherConnectedTo->orientation == connectedTo->orientation );
            }
            else {
                std::cerr << "Inconsistent state (couldn't find module " << connectedTo->moduleId
                          << ", connector " << connectedTo->connector << ")\n";
            }
        }
        return { connectedTo };
    }
    return std::nullopt;
}

bool ModuleStates::setConnectorPower( ModuleId moduleId,
                                      int connector,
                                      ConnectorLine line,
                                      bool connect )
{
    using ConnectorCmd = rofi::messages::ConnectorCmd;

    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        switch ( line ) {
            case ConnectorCmd::INT_LINE:
                connectorInnerState->get().internal() = connect;
                return true;
            case ConnectorCmd::EXT_LINE:
                connectorInnerState->get().external() = connect;
                return true;
            default:
                return false;
        }
    }
    return false;
}

std::map< ModuleId, ModuleInnerState > ModuleStates::innerStatesFromConfiguration(
        const Rofibot & rofibotConfiguration )
{
    auto innerStates = std::map< ModuleId, ModuleInnerState >();
    for ( const auto & moduleInfo : rofibotConfiguration.modules() ) {
        auto & _module = *moduleInfo.module.get();

        // TODO get only user-configurable joints from roficom
        size_t jointCount = 0;
        for ( auto joints = _module.joints(); jointCount < joints.size(); jointCount++ ) {
            if ( joints[ jointCount ].joint->jointLimits().empty() ) {
                break;
            }
        }
        auto moduleInnerState = ModuleInnerState( jointCount, _module.connectors().size() );

        auto [ it, success ] = innerStates.emplace( _module.getId(),
                                                    std::move( moduleInnerState ) );
        if ( !success ) {
            throw std::runtime_error( "Multiple same module ids in configuration" );
        }
    }

    return innerStates;
}
