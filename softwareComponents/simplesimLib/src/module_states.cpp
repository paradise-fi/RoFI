#include "simplesim/module_states.hpp"


using namespace rofi::configuration;
using namespace rofi::simplesim;


std::optional< ModuleStates::RofiDescription > ModuleStates::getDescription(
        ModuleId moduleId ) const
{
    if ( auto * moduleInnerState = getModuleInnerState( moduleId ) ) {
        RofiDescription description;
        size_t jointCount = moduleInnerState->joints().size();
        size_t connectorCount = moduleInnerState->connectors().size();
        assert( jointCount <= INT_MAX );
        assert( connectorCount <= INT_MAX );
        description.set_jointcount( static_cast< int >( jointCount ) );
        description.set_connectorcount( static_cast< int >( connectorCount ) );
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
    if ( auto * jointInnerState = getJointInnerState( moduleId, joint ) ) {
        return jointInnerState->velocity();
    }
    return std::nullopt;
}

bool ModuleStates::setPositionControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointPositionControl positionControl )
{
    if ( auto * jointInnerState = getJointInnerState( moduleId, joint ) ) {
        jointInnerState->setPositionControl( std::move( positionControl ) );
        return true;
    }
    return false;
}

bool ModuleStates::setVelocityControl( ModuleId moduleId,
                                       int joint,
                                       ModuleStates::JointVelocityControl velocityControl )
{
    if ( auto * jointInnerState = getJointInnerState( moduleId, joint ) ) {
        jointInnerState->setVelocityControl( std::move( velocityControl ) );
        return true;
    }
    return false;
}

std::optional< rofi::messages::ConnectorState > ModuleStates::getConnectorState(
        ModuleId moduleId,
        int connector ) const
{
    if ( auto * connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        return connectorInnerState->connectorState();
    }
    return {};
}

std::optional< ModuleStates::ConnectedTo > ModuleStates::getConnectedTo( ModuleId moduleId,
                                                                         int connector ) const
{
    if ( auto * connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        return connectorInnerState->connectedTo();
    }
    return {};
}

bool ModuleStates::extendConnector( ModuleId moduleId, int connector )
{
    if ( auto * connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        connectorInnerState->setExtending();
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
    if ( auto * connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        auto connectedTo = connectorInnerState->resetConnectedTo();
        connectorInnerState->setRetracting();

        if ( connectedTo ) {
            if ( auto * otherConnInner = getConnectorInnerState( connectedTo->moduleId,
                                                                 connectedTo->connector ) )
            {
                [[maybe_unused]] auto otherConnectedTo = otherConnInner->resetConnectedTo();

                assert( otherConnectedTo );
                assert( otherConnectedTo->moduleId == moduleId );
                assert( otherConnectedTo->connector == connector );
                assert( otherConnectedTo->orientation == connectedTo->orientation );
            } else {
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

    if ( auto * connectorInnerState = getConnectorInnerState( moduleId, connector ) ) {
        switch ( line ) {
            case ConnectorCmd::INT_LINE:
                connectorInnerState->internal() = connect;
                return true;
            case ConnectorCmd::EXT_LINE:
                connectorInnerState->external() = connect;
                return true;
            default:
                return false;
        }
    }
    return false;
}

auto ModuleStates::computeNextIteration( std::chrono::duration< float > simStepTime ) const
        -> std::pair< ModuleStates::RofibotConfigurationPtr,
                      std::vector< ModuleStates::PositionReached > >
{
    auto currentConfig = currentConfiguration();
    assert( currentConfig );
    auto newConfiguration = std::make_shared< Rofibot >( *currentConfig );
    assert( newConfiguration );

    std::vector< PositionReached > positionsReached;
    for ( auto & moduleInfo : newConfiguration->modules() ) {
        assert( moduleInfo.module.get() );
        auto & module_ = *moduleInfo.module;

        auto * moduleInnerState = getModuleInnerState( module_.getId() );
        assert( moduleInnerState );


        // Update joint positions
        std::span jointInnerStates = moduleInnerState->joints();
        std::ranges::view auto jointConfigurations = module_.configurableJoints();
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
                                                                                   simStepTime );
            if ( posReached ) {
                assert( i < INT_MAX );
                positionsReached.push_back( PositionReached{ .moduleId = module_.getId(),
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

    newConfiguration->prepare();
    if ( auto [ ok, err ] = newConfiguration->isValid( rofi::configuration::SimpleCollision{} );
         !ok ) {
        throw std::runtime_error( err );
    }
    return std::pair( newConfiguration, std::move( positionsReached ) );
}

std::map< ModuleId, ModuleInnerState > ModuleStates::innerStatesFromConfiguration(
        const Rofibot & rofibotConfiguration )
{
    auto innerStates = std::map< ModuleId, ModuleInnerState >();
    for ( const auto & moduleInfo : rofibotConfiguration.modules() ) {
        const auto & _module = *moduleInfo.module;

        auto joints = std::ranges::distance( _module.configurableJoints() );
        auto connectors = _module.connectors().size();
        assert( joints < INT_MAX );
        assert( connectors < INT_MAX );
        auto moduleInnerState = ModuleInnerState( static_cast< int >( joints ),
                                                  static_cast< int >( connectors ) );

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
