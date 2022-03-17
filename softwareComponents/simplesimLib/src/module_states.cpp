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

std::optional< rofi::messages::ConnectorState > ModuleStates::getConnectorState(
        ModuleId moduleId,
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
        connectorInnerState->get().setExtending();
        return true;
    }
    return false;
}

bool ModuleStates::connectConnectors( ModuleId lhsModuleId,
                                      int lhsConnector,
                                      ModuleId rhsModuleId,
                                      int rhsConnector,
                                      ConnectorInnerState::Orientation orientation )
{
    using rofi::simplesim::ConnectorInnerState;

    if ( auto lhsConnectorInnerStateOpt = getConnectorInnerState( lhsModuleId, lhsConnector ) ) {
        ConnectorInnerState & lhsConnectorInnerState = *lhsConnectorInnerStateOpt;
        if ( auto rhsConnectorInnerStateOpt = getConnectorInnerState( rhsModuleId, rhsConnector ) )
        {
            ConnectorInnerState & rhsConnectorInnerState = *rhsConnectorInnerStateOpt;
            lhsConnectorInnerState.setConnectedTo( rhsModuleId, rhsConnector, orientation );
            rhsConnectorInnerState.setConnectedTo( lhsModuleId, lhsConnector, orientation );
            return true;
        }
    }
    return false;
}

std::optional< ModuleStates::ConnectedTo > ModuleStates::retractConnector( ModuleId moduleId,
                                                                           int connector )
{
    if ( auto connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        auto connectedTo = connectorInnerState->get().resetConnectedTo();
        connectorInnerState->get().setRetracting();

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


std::pair< std::shared_ptr< const rofi::configuration::Rofibot >,
           std::vector< ModuleStates::PositionReached > >
        ModuleStates::computeNextIteration( std::chrono::duration< float > duration ) const
{
    auto new_configuration = _physicalModulesConfiguration.visit( []( const auto & configPtr ) {
        assert( configPtr );
        return std::make_shared< rofi::configuration::Rofibot >( *configPtr );
    } );

    std::vector< PositionReached > positionsReached;
    assert( new_configuration );
    for ( auto & moduleInfo : new_configuration->modules() ) {
        assert( moduleInfo.module.get() );
        auto & module_ = moduleInfo.module;

        auto moduleInnerStateOpt = getModuleInnerState( module_->getId() );
        assert( moduleInnerStateOpt );
        auto & moduleInnerState = moduleInnerStateOpt->get();

        std::span jointInnerStates = moduleInnerState.joints();
        std::ranges::view auto jointConfigurations = module_->configurableJoints();
        assert( std::ssize( jointInnerStates ) == std::ranges::distance( jointConfigurations ) );

        auto enumerated = std::views::transform( [ i = 0UL ]< typename T >( T && value ) mutable {
            return std::tuple< T, size_t >( std::forward< T >( value ), i++ );
        } );
        for ( auto [ jointConfiguration, i ] : jointConfigurations | enumerated ) {
            static_assert(
                    std::is_same_v< decltype( jointConfiguration ), configuration::Joint & > );
            const auto & jointInnerState = jointInnerStates[ i ];

            assert( jointConfiguration.positions().size() == 1 );
            assert( jointConfiguration.jointLimits().size() == 1 );
            auto currentPosition = jointConfiguration.positions().front();
            auto jointLimits = jointConfiguration.jointLimits().front();

            auto [ posReached, newPosition ] = jointInnerState.computeNewPosition( currentPosition,
                                                                                   duration );
            if ( posReached ) {
                positionsReached.push_back( PositionReached{ .moduleId = module_->getId(),
                                                             .joint = i,
                                                             .position = newPosition } );
            }

            assert( jointLimits.first <= jointLimits.second );
            auto clampedNewPosition = std::clamp( newPosition,
                                                  jointLimits.first,
                                                  jointLimits.second );

            jointConfiguration.setPositions( std::array{ clampedNewPosition } );
        }
    }

    new_configuration->prepare();
    if ( auto [ ok, err ] = new_configuration->isValid( rofi::configuration::SimpleCollision{} );
         !ok ) {
        throw std::runtime_error( err );
    }
    return std::pair( new_configuration, std::move( positionsReached ) );
}

std::map< ModuleId, ModuleInnerState > ModuleStates::innerStatesFromConfiguration(
        const Rofibot & rofibotConfiguration )
{
    auto innerStates = std::map< ModuleId, ModuleInnerState >();
    for ( const auto & moduleInfo : rofibotConfiguration.modules() ) {
        const auto & _module = *moduleInfo.module;

        auto moduleInnerState = ModuleInnerState( std::ranges::distance(
                                                          _module.configurableJoints() ),
                                                  _module.connectors().size() );

#ifdef VERBOSE
        std::cerr << "Module id: " << _module.getId()
                  << ", joints: " << moduleInnerState.joints().size()
                  << ", connectors: " << moduleInnerState.connectors().size() << std::endl;
#endif

        auto [ it, success ] = innerStates.emplace( _module.getId(),
                                                    std::move( moduleInnerState ) );
        if ( !success ) {
            throw std::runtime_error( "Multiple same module ids in configuration" );
        }
    }

    for ( auto & connection : rofibotConfiguration.roficomConnections() ) {
        ModuleId sourceModuleId = connection.getSourceModule( rofibotConfiguration ).getId();
        ModuleId destModuleId = connection.getDestModule( rofibotConfiguration ).getId();

        assert( innerStates.contains( sourceModuleId ) );
        std::span sourceConnectors = innerStates.at( sourceModuleId ).connectors();

        assert( to_unsigned( connection.sourceConnector ) < sourceConnectors.size() );
        auto & sourceConnector = sourceConnectors[ connection.sourceConnector ];

        assert( !sourceConnector.connectedTo().has_value() );
        sourceConnector.setExtendedWithoutConnecting();
        sourceConnector.setConnectedTo( destModuleId,
                                        connection.destConnector,
                                        connection.orientation );


        assert( innerStates.contains( destModuleId ) );
        std::span destConnectors = innerStates.at( destModuleId ).connectors();

        assert( to_unsigned( connection.destConnector ) < destConnectors.size() );
        auto & destConnector = destConnectors[ connection.destConnector ];

        assert( !destConnector.connectedTo().has_value() );
        destConnector.setExtendedWithoutConnecting();
        destConnector.setConnectedTo( sourceModuleId,
                                      connection.sourceConnector,
                                      connection.orientation );
    }

    return innerStates;
}
