#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <optional>

#include "atoms/guarded.hpp"
#include "configuration/rofibot.hpp"
#include "inner_state.hpp"

#include <connectorCmd.pb.h>
#include <jointCapabilities.pb.h>
#include <rofiDescription.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
namespace detail
{
    using ModuleInnerStates = std::map< ModuleId, ModuleInnerState >;
    inline auto getModuleInnerState( const ModuleInnerStates & moduleStates, ModuleId moduleId )
            -> const ModuleInnerState *
    {
        auto innerState = moduleStates.find( moduleId );
        if ( innerState == moduleStates.end() ) {
            return nullptr;
        }

        return &innerState->second;
    }
    inline auto getModuleInnerState( ModuleInnerStates & moduleStates, ModuleId moduleId )
            -> ModuleInnerState *
    {
        auto innerState = moduleStates.find( moduleId );
        if ( innerState == moduleStates.end() ) {
            return nullptr;
        }

        return &innerState->second;
    }

    inline auto getConnectorInnerState( const ModuleInnerStates & moduleStates, Connector conn )
            -> const ConnectorInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleStates, conn.moduleId ) ) {
            auto connectorInnerStates = moduleInnerState->connectors();
            if ( conn.connIdx >= 0 && to_unsigned( conn.connIdx ) < connectorInnerStates.size() ) {
                return &connectorInnerStates[ conn.connIdx ];
            }
        }
        return nullptr;
    }
    inline auto getConnectorInnerState( ModuleInnerStates & moduleStates, Connector conn )
            -> ConnectorInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleStates, conn.moduleId ) ) {
            auto connectorInnerStates = moduleInnerState->connectors();
            if ( conn.connIdx >= 0 && to_unsigned( conn.connIdx ) < connectorInnerStates.size() ) {
                return &connectorInnerStates[ conn.connIdx ];
            }
        }
        return nullptr;
    }

    inline auto getJointInnerState( const ModuleInnerStates & moduleStates, Joint joint )
            -> const JointInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleStates, joint.moduleId ) ) {
            auto jointInnerStates = moduleInnerState->joints();
            if ( joint.jointIdx >= 0 && to_unsigned( joint.jointIdx ) < jointInnerStates.size() ) {
                return &jointInnerStates[ joint.jointIdx ];
            }
        }
        return nullptr;
    }
    inline auto getJointInnerState( ModuleInnerStates & moduleStates, Joint joint )
            -> JointInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleStates, joint.moduleId ) ) {
            auto jointInnerStates = moduleInnerState->joints();
            if ( joint.jointIdx >= 0 && to_unsigned( joint.jointIdx ) < jointInnerStates.size() ) {
                return &jointInnerStates[ joint.jointIdx ];
            }
        }
        return nullptr;
    }

    inline auto getJointConfig( const rofi::configuration::Rofibot & rofibot, Joint joint )
            -> const rofi::configuration::Joint *
    {
        if ( auto * moduleConfig = rofibot.getModule( joint.moduleId ) ) {
            if ( joint.jointIdx < 0 ) {
                return nullptr;
            }
            auto joints = moduleConfig->configurableJoints() | std::views::drop( joint.jointIdx );
            return joints.empty() ? nullptr : &joints.front();
        }
        return nullptr;
    }
    inline auto getJointConfig( rofi::configuration::Rofibot & rofibot, Joint joint )
            -> rofi::configuration::Joint *
    {
        if ( auto * moduleConfig = rofibot.getModule( joint.moduleId ) ) {
            if ( joint.jointIdx < 0 ) {
                return nullptr;
            }
            auto joints = moduleConfig->configurableJoints() | std::views::drop( joint.jointIdx );
            return joints.empty() ? nullptr : &joints.front();
        }
        return nullptr;
    }

    struct ConfigurationUpdateEvents {
        class PositionReached {
        public:
            Joint joint;
            float position;

            auto getRofiResp() const -> rofi::messages::RofiResp
            {
                return joint.getRofiResp( rofi::messages::JointCmd::SET_POS_WITH_SPEED, position );
            }
        };

        class ConnectionChanged {
        public:
            Connector lhs;
            Connector rhs;
            std::optional< rofi::configuration::roficom::Orientation > orientation;

            /// Updates the connection of the inner connector states.
            ///
            /// If `orientation` has a value, it connects the connectors with this orientation.
            /// Otherwise disconnects them.
            ///
            /// Requires that `lhs != rhs`.
            void updateInnerStates(
                    std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
            {
                if ( orientation ) {
                    connectInnerStates( moduleInnerStates );
                } else {
                    resetInnerStates( moduleInnerStates );
                }
            }

        private:
            /// Connects the inner connector states.
            /// Requires that `lhs != rhs` and that `orientation` has value.
            void connectInnerStates(
                    std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
            {
                assert( lhs != rhs );
                assert( orientation );
                auto * lhsInnerState = getConnectorInnerState( moduleInnerStates, lhs );
                auto * rhsInnerState = getConnectorInnerState( moduleInnerStates, rhs );
                assert( lhsInnerState );
                assert( rhsInnerState );

                if ( lhsInnerState->connectedTo().has_value()
                     || rhsInnerState->connectedTo().has_value() ) {
                    using ConnectedToValue = ConnectorInnerState::OtherConnector;
                    assert( lhsInnerState->connectedTo().has_value() );
                    assert( rhsInnerState->connectedTo().has_value() );
                    assert( *lhsInnerState->connectedTo()
                            == ConnectedToValue( rhs, *orientation ) );
                    assert( *rhsInnerState->connectedTo()
                            == ConnectedToValue( lhs, *orientation ) );
                }

                lhsInnerState->setConnectedTo( rhs, *orientation );
                rhsInnerState->setConnectedTo( lhs, *orientation );
            }
            /// Disconnects the inner connector states.
            /// Requires that `lhs != rhs` and that `orientation` has no value.
            void resetInnerStates(
                    std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
            {
                assert( lhs != rhs );
                assert( orientation == std::nullopt );
                auto * lhsInnerState = getConnectorInnerState( moduleInnerStates, lhs );
                auto * rhsInnerState = getConnectorInnerState( moduleInnerStates, rhs );
                assert( lhsInnerState );
                assert( rhsInnerState );

                [[maybe_unused]] auto lhsConnectedTo = lhsInnerState->resetConnectedTo();
                [[maybe_unused]] auto rhsConnectedTo = rhsInnerState->resetConnectedTo();

                if ( lhsConnectedTo.has_value() || rhsConnectedTo.has_value() ) {
                    assert( lhsConnectedTo.has_value() );
                    assert( rhsConnectedTo.has_value() );
                    assert( lhsConnectedTo->connector == rhs );
                    assert( rhsConnectedTo->connector == lhs );
                    assert( lhsConnectedTo->orientation == rhsConnectedTo->orientation );
                }
            }
        };

        std::vector< PositionReached > positionsReached;
        std::vector< ConnectionChanged > connectionsChanged;
        std::vector< Connector > connectorsToFinilizePosition;
    };

    inline auto connectToNearbyConnector( const rofi::configuration::Component & roficom )
            -> std::optional< ConfigurationUpdateEvents::ConnectionChanged >
    {
        using CUE = ConfigurationUpdateEvents;
        assert( roficom.type == rofi::configuration::ComponentType::Roficom );

        if ( auto nearConnectorPair = roficom.getNearConnector() ) {
            const auto & [ nearConnector, connectionOrientation ] = *nearConnectorPair;
            assert( nearConnector.type == rofi::configuration::ComponentType::Roficom );
            configuration::connect( roficom, nearConnector, connectionOrientation );

            assert( roficom.parent );
            assert( nearConnector.parent );
            return CUE::ConnectionChanged{
                    .lhs = Connector{ .moduleId = roficom.parent->getId(),
                                      .connIdx = roficom.getIndexInParent() },
                    .rhs = Connector{ .moduleId = nearConnector.parent->getId(),
                                      .connIdx = nearConnector.getIndexInParent() },
                    .orientation = { connectionOrientation } };
        }
        return std::nullopt;
    }
} // namespace detail


class ModuleStates {
    static constexpr float maxJointSpeed = 1.;

public:
    using RofiDescription = rofi::messages::RofiDescription;
    using RofiResp = rofi::messages::RofiResp;
    using ConnectorLine = rofi::messages::ConnectorCmd::Line;
    using ConnectedTo = std::optional< ConnectorInnerState::OtherConnector >;
    using JointCapabilities = rofi::messages::JointCapabilities;
    using JointVelocityControl = JointInnerState::VelocityControl;
    using JointPositionControl = JointInnerState::PositionControl;
    using RofibotConfigurationPtr = std::shared_ptr< const rofi::configuration::Rofibot >;


    explicit ModuleStates( RofibotConfigurationPtr rofibotConfiguration )
            : _physicalModulesConfiguration(
                    rofibotConfiguration
                            ? std::move( rofibotConfiguration )
                            : std::make_shared< const rofi::configuration::Rofibot >() )
            , _moduleInnerStates(
                      this->_physicalModulesConfiguration.visit( []( auto & configPtr ) {
                          assert( configPtr );
                          return initInnerStatesFromConfiguration( *configPtr );
                      } ) )
    {
        assert( _physicalModulesConfiguration.visit( []( const auto & configuration ) {
            assert( configuration );
            return configuration->isValid( rofi::configuration::SimpleCollision() ).first;
        } ) );
    }

    // Returns rofi description if module with moduleId exists
    std::optional< RofiDescription > getDescription( ModuleId moduleId ) const;


    // Returns joint position limits if module with given joint exists
    std::optional< std::pair< float, float > > getJointPositionLimits( Joint joint ) const;
    // Returns joint capabilities if module with given joint exists
    std::optional< JointCapabilities > getJointCapabilities( Joint joint ) const;
    // Returns joint position if module with given joint exists
    std::optional< float > getJointPosition( Joint joint ) const;
    // Returns joint velocity if module with given joint exists
    std::optional< float > getJointVelocity( Joint joint ) const;

    // Sets joint position control and returns true if module with given joint exists
    bool setPositionControl( Joint joint, JointPositionControl positionControl );
    // Sets joint velocity control and returns true if module with given joint exists
    bool setVelocityControl( Joint joint, JointVelocityControl velocityControl );


    // Returns connector state if module with given connector exists
    std::optional< rofi::messages::ConnectorState > getConnectorState( Connector connector ) const;
    // Returns the connectedTo connector if such exists
    std::optional< ConnectedTo > getConnectedTo( Connector connector ) const;

    // Extends connector and returns true if module with given connector exists
    bool extendConnector( Connector connector );
    // Retracts connector and returns previously connected to if module with given connector exists
    std::optional< ConnectedTo > retractConnector( Connector connector );
    // Sets given power line in connector and returns true if module with given connector exists
    bool setConnectorPower( Connector connector, ConnectorLine line, bool connect );

    std::set< ModuleId > getModuleIds() const
    {
        // Change to range in C++20
        auto moduleIds = std::set< ModuleId >();
        for ( auto & [ moduleId, _value ] : _moduleInnerStates ) {
            moduleIds.insert( moduleId );
        }
        return moduleIds;
    }


    template < std::invocable< RofiResp > Callback >
    auto updateToNextIteration( std::chrono::duration< float > simStepTime,
                                Callback onRespCallback ) -> RofibotConfigurationPtr
    {
        auto [ newConfiguration, updateEvents ] = computeNextIteration( simStepTime );
        assert( newConfiguration );

        auto oldConfiguration = _physicalModulesConfiguration.visit(
                [ &newConfiguration ]( auto & configuration ) {
                    auto oldConfiguration = std::move( configuration );
                    configuration = newConfiguration;
                    return oldConfiguration;
                } );

        _configurationHistory->push_back( std::move( oldConfiguration ) );

        for ( const auto & posReached : updateEvents.positionsReached ) {
            if ( auto jointInnerPos = detail::getJointInnerState( _moduleInnerStates,
                                                                  posReached.joint ) ) {
                jointInnerPos->holdCurrentPosition();
            }
            onRespCallback( posReached.getRofiResp() );
        }
        for ( const auto & connection : updateEvents.connectionsChanged ) {
            connection.updateInnerStates( _moduleInnerStates );
        }
        for ( const auto & connector : updateEvents.connectorsToFinilizePosition ) {
            if ( auto * connInnerState = detail::getConnectorInnerState( _moduleInnerStates,
                                                                         connector ) ) {
                connInnerState->finilizePosition();
            }
        }
        return std::move( newConfiguration );
    }

    auto currentConfiguration() const -> RofibotConfigurationPtr
    {
        auto result = _physicalModulesConfiguration.copy();
        assert( result );
        return result;
    }

private:
    auto computeNextIteration( std::chrono::duration< float > simStepTime ) const
            -> std::pair< RofibotConfigurationPtr, detail::ConfigurationUpdateEvents >;

    static auto initInnerStatesFromConfiguration(
            const rofi::configuration::Rofibot & rofibotConfiguration )
            -> std::map< ModuleId, ModuleInnerState >;

private:
    atoms::Guarded< RofibotConfigurationPtr > _physicalModulesConfiguration;
    std::map< ModuleId, ModuleInnerState > _moduleInnerStates;

    atoms::Guarded< std::vector< RofibotConfigurationPtr > > _configurationHistory;
};

} // namespace rofi::simplesim
