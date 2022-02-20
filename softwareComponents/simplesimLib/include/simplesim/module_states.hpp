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
    struct ConfigurationUpdateEvents {
        class PositionReached {
        public:
            ModuleId moduleId;
            int joint;
            float position;

            auto getInnerState( std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
                    -> JointInnerState &
            {
                auto moduleStateIt = moduleInnerStates.find( moduleId );
                assert( moduleStateIt != moduleInnerStates.end() );

                std::span joints = moduleStateIt->second.joints();
                assert( joint >= 0 && to_unsigned( joint ) < joints.size() );
                return joints[ joint ];
            }

            auto getRofiResp() const -> rofi::messages::RofiResp
            {
                rofi::messages::RofiResp rofiResp;
                rofiResp.set_rofiid( moduleId );
                rofiResp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );
                auto & jointResp = *rofiResp.mutable_jointresp();
                jointResp.set_joint( joint );
                jointResp.set_resptype( rofi::messages::JointCmd::SET_POS_WITH_SPEED );
                jointResp.set_value( position );
                return rofiResp;
            }
        };

        struct Connector {
            ModuleId moduleId;
            int connIdx;

            bool operator==( const Connector & ) const = default;

            auto getInnerState( std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
                    -> ConnectorInnerState &
            {
                auto moduleStateIt = moduleInnerStates.find( moduleId );
                assert( moduleStateIt != moduleInnerStates.end() );

                std::span connectors = moduleStateIt->second.connectors();
                assert( connIdx >= 0 && to_unsigned( connIdx ) < connectors.size() );
                return connectors[ connIdx ];
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

            /// Connects the inner connector states.
            /// Requires that `lhs != rhs` and that `orientation` has value.
            void connectInnerStates(
                    std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
            {
                assert( lhs != rhs );
                assert( orientation );
                auto & lhsInner = lhs.getInnerState( moduleInnerStates );
                auto & rhsInner = rhs.getInnerState( moduleInnerStates );

                if ( lhsInner.connectedTo().has_value() || rhsInner.connectedTo().has_value() ) {
                    using ConnectedToValue = ConnectorInnerState::OtherConnector;
                    assert( lhsInner.connectedTo().has_value() );
                    assert( rhsInner.connectedTo().has_value() );
                    assert( lhsInner.connectedTo()
                            == ConnectedToValue( rhs.moduleId, rhs.connIdx, *orientation ) );
                    assert( rhsInner.connectedTo()
                            == ConnectedToValue( lhs.moduleId, lhs.connIdx, *orientation ) );
                }

                lhsInner.setConnectedTo( rhs.moduleId, rhs.connIdx, *orientation );
                rhsInner.setConnectedTo( lhs.moduleId, lhs.connIdx, *orientation );
            }
            /// Disconnects the inner connector states.
            /// Requires that `lhs != rhs` and that `orientation` has no value.
            void resetInnerStates(
                    std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
            {
                assert( lhs != rhs );
                assert( !orientation );

                [[maybe_unused]] auto lhsConnectedTo = lhs.getInnerState( moduleInnerStates )
                                                               .resetConnectedTo();
                [[maybe_unused]] auto rhsConnectedTo = rhs.getInnerState( moduleInnerStates )
                                                               .resetConnectedTo();

                if ( lhsConnectedTo.has_value() || rhsConnectedTo.has_value() ) {
                    assert( lhsConnectedTo.has_value() );
                    assert( rhsConnectedTo.has_value() );
                    assert( lhsConnectedTo->moduleId == rhs.moduleId
                            && lhsConnectedTo->connector == rhs.connIdx );
                    assert( rhsConnectedTo->moduleId == lhs.moduleId
                            && rhsConnectedTo->connector == lhs.connIdx );
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
                    .lhs = CUE::Connector{ .moduleId = roficom.parent->getId(),
                                           .connIdx = roficom.getIndexInParent() },
                    .rhs = CUE::Connector{ .moduleId = nearConnector.parent->getId(),
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
                          return innerStatesFromConfiguration( *configPtr );
                      } ) )
    {
        assert( _physicalModulesConfiguration.visit( []( const auto & configuration ) {
            assert( configuration );
            return configuration->isValid( rofi::configuration::SimpleCollision() ).first;
        } ) );
    }

    // Returns rofi description if module with moduleId exists
    std::optional< RofiDescription > getDescription( ModuleId moduleId ) const;


    // Returns joint position limits if module with moduleId exists and has given joint
    std::optional< std::pair< float, float > > getJointPositionLimits( ModuleId moduleId,
                                                                       int joint ) const;
    // Returns joint capabilities if module with moduleId exists and has given joint
    std::optional< JointCapabilities > getJointCapabilities( ModuleId moduleId, int joint ) const;
    // Returns joint position if module with moduleId exists and has given joint
    std::optional< float > getJointPosition( ModuleId moduleId, int joint ) const;
    // Returns joint velocity if module with moduleId exists and has given joint
    std::optional< float > getJointVelocity( ModuleId moduleId, int joint ) const;

    // Sets joint position control and returns true if module with moduleId exists and has given joint
    bool setPositionControl( ModuleId moduleId, int joint, JointPositionControl positionControl );
    // Sets joint velocity control and returns true if module with moduleId exists and has given joint
    bool setVelocityControl( ModuleId moduleId, int joint, JointVelocityControl velocityControl );


    // Returns connector state if module with moduleId exists and has given connector
    std::optional< rofi::messages::ConnectorState > getConnectorState( ModuleId moduleId,
                                                                       int connector ) const;
    // Returns the connectedTo connector if such exists
    std::optional< ConnectedTo > getConnectedTo( ModuleId moduleId, int connector ) const;

    // Extends connector and returns true if module with moduleId exists and has given connector
    bool extendConnector( ModuleId moduleId, int connector );
    // Retracts connector and returns previously connected to if module with moduleId exists and has given connector
    std::optional< ConnectedTo > retractConnector( ModuleId moduleId, int connector );
    // Sets given power line in connector and returns true if module with moduleId exists and has given connector
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
            posReached.getInnerState( _moduleInnerStates ).holdCurrentPosition();
            onRespCallback( posReached.getRofiResp() );
        }
        for ( const auto & connection : updateEvents.connectionsChanged ) {
            connection.updateInnerStates( _moduleInnerStates );
        }
        for ( const auto & connector : updateEvents.connectorsToFinilizePosition ) {
            connector.getInnerState( _moduleInnerStates ).finilizePosition();
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

    auto getModuleInnerState( ModuleId moduleId ) const -> const ModuleInnerState *
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return nullptr;
        }

        return &innerState->second;
    }
    auto getModuleInnerState( ModuleId moduleId ) -> ModuleInnerState *
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return nullptr;
        }

        return &innerState->second;
    }

    auto getConnectorInnerState( ModuleId moduleId, int connector ) const
            -> const ConnectorInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleId ) ) {
            auto connectorInnerStates = moduleInnerState->connectors();
            if ( connector >= 0 && to_unsigned( connector ) < connectorInnerStates.size() ) {
                return &connectorInnerStates[ connector ];
            }
        }
        return nullptr;
    }
    auto getConnectorInnerState( ModuleId moduleId, int connector ) -> ConnectorInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleId ) ) {
            auto connectorInnerStates = moduleInnerState->connectors();
            if ( connector >= 0 && to_unsigned( connector ) < connectorInnerStates.size() ) {
                return &connectorInnerStates[ connector ];
            }
        }
        return nullptr;
    }

    auto getJointInnerState( ModuleId moduleId, int joint ) const -> const JointInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleId ) ) {
            auto jointInnerStates = moduleInnerState->joints();
            if ( joint >= 0 && to_unsigned( joint ) < jointInnerStates.size() ) {
                return &jointInnerStates[ joint ];
            }
        }
        return nullptr;
    }
    auto getJointInnerState( ModuleId moduleId, int joint ) -> JointInnerState *
    {
        if ( auto * moduleInnerState = getModuleInnerState( moduleId ) ) {
            auto jointInnerStates = moduleInnerState->joints();
            if ( joint >= 0 && to_unsigned( joint ) < jointInnerStates.size() ) {
                return &jointInnerStates[ joint ];
            }
        }
        return nullptr;
    }

    static std::map< ModuleId, ModuleInnerState > innerStatesFromConfiguration(
            const rofi::configuration::Rofibot & rofibotConfiguration );

private:
    atoms::Guarded< RofibotConfigurationPtr > _physicalModulesConfiguration;
    std::map< ModuleId, ModuleInnerState > _moduleInnerStates;

    atoms::Guarded< std::vector< RofibotConfigurationPtr > > _configurationHistory;
};

} // namespace rofi::simplesim
