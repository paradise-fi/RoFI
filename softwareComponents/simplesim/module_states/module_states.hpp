#pragma once

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
class ModuleStates
{
    static constexpr float maxJointSpeed = 1.;

public:
    using RofiDescription = rofi::messages::RofiDescription;
    using RofiResp = rofi::messages::RofiResp;
    using ConnectorState = ConnectorInnerState::ConnectorState;
    using ConnectorLine = rofi::messages::ConnectorCmd::Line;
    using ConnectedTo = std::optional< ConnectorInnerState::OtherConnector >;
    using JointCapabilities = rofi::messages::JointCapabilities;
    using JointVelocityControl = JointInnerState::VelocityControl;
    using JointPositionControl = JointInnerState::PositionControl;


    explicit ModuleStates(
            std::shared_ptr< const rofi::configuration::Rofibot > rofibotConfiguration )
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
        // assert( _physicalModulesConfiguration->isValid() ); // TODO `isValid` should be const
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
    std::optional< ConnectorState > getConnectorState( ModuleId moduleId, int connector ) const;
    // Returns the connectedTo connector if such exists
    std::optional< ConnectedTo > getConnectedTo( ModuleId moduleId, int connector ) const;

    // Extends connector and returns true if module with moduleId exists and has given connector
    bool extendConnector( ModuleId moduleId, int connector );
    // Retracts connector and returns true if module with moduleId exists and has given connector
    bool retractConnector( ModuleId moduleId, int connector );
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
    std::shared_ptr< const rofi::configuration::Rofibot > updateToNextIteration(
            std::chrono::duration< float > duration,
            Callback onRespCallback )
    {
        auto [ new_configuration, positionsReached ] = computeNextIteration( duration );
        assert( new_configuration );

        auto old_configuration = _physicalModulesConfiguration.visit(
                [ &new_configuration ]( auto & configuration ) {
                    auto old_configuration = std::move( configuration );
                    configuration = new_configuration;
                    return old_configuration;
                } );

        _configurationHistory->push_back( std::move( old_configuration ) );

        for ( const auto & posReached : positionsReached ) {
            posReached.getInnerState( _moduleInnerStates ).holdCurrentPosition();
            onRespCallback( posReached.getRofiResp() );
        }
        return std::move( new_configuration );
    }


private:
    std::optional< std::reference_wrapper< const ModuleInnerState > > getModuleInnerState(
            ModuleId moduleId ) const
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return {};
        }

        return innerState->second;
    }
    std::optional< std::reference_wrapper< ModuleInnerState > > getModuleInnerState(
            ModuleId moduleId )
    {
        auto innerState = _moduleInnerStates.find( moduleId );
        if ( innerState == _moduleInnerStates.end() ) {
            return {};
        }

        return innerState->second;
    }

    std::optional< std::reference_wrapper< const ConnectorInnerState > > getConnectorInnerState(
            ModuleId moduleId,
            int connector ) const
    {
        auto moduleInnerState = getModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto connectorInnerStates = moduleInnerState->get().connectors();
        if ( connector < 0 || size_t( connector ) >= connectorInnerStates.size() ) {
            return {};
        }
        return connectorInnerStates[ connector ];
    }
    std::optional< std::reference_wrapper< ConnectorInnerState > > getConnectorInnerState(
            ModuleId moduleId,
            int connector )
    {
        auto moduleInnerState = getModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto connectorInnerStates = moduleInnerState->get().connectors();
        if ( connector < 0 || size_t( connector ) >= connectorInnerStates.size() ) {
            return {};
        }
        return connectorInnerStates[ connector ];
    }

    std::optional< std::reference_wrapper< const JointInnerState > > getJointInnerState(
            ModuleId moduleId,
            int joint ) const
    {
        auto moduleInnerState = getModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto jointInnerStates = moduleInnerState->get().joints();
        if ( joint < 0 || size_t( joint ) >= jointInnerStates.size() ) {
            return {};
        }
        return jointInnerStates[ joint ];
    }
    std::optional< std::reference_wrapper< JointInnerState > > getJointInnerState(
            ModuleId moduleId,
            int joint )
    {
        auto moduleInnerState = getModuleInnerState( moduleId );
        if ( !moduleInnerState ) {
            return {};
        }
        auto jointInnerStates = moduleInnerState->get().joints();
        if ( joint < 0 || size_t( joint ) >= jointInnerStates.size() ) {
            return {};
        }
        return jointInnerStates[ joint ];
    }


    class PositionReached
    {
    public:
        ModuleId moduleId;
        size_t joint;
        float position;

        JointInnerState & getInnerState(
                std::map< ModuleId, ModuleInnerState > & moduleInnerStates ) const
        {
            auto moduleStateIt = moduleInnerStates.find( moduleId );
            assert( moduleStateIt != moduleInnerStates.end() );

            std::span joints = moduleStateIt->second.joints();
            assert( joint < joints.size() );
            return joints[ joint ];
        }

        rofi::messages::RofiResp getRofiResp() const
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

    std::pair< std::shared_ptr< const rofi::configuration::Rofibot >,
               std::vector< PositionReached > >
            computeNextIteration( std::chrono::duration< float > duration ) const
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
            tcb::span jointConfigurations = module_->joints();
            assert( jointInnerStates.size() == jointConfigurations.size() );
            for ( size_t i = 0; i < jointInnerStates.size(); i++ ) {
                assert( jointConfigurations[ i ].joint.get() );
                auto & jointConfiguration = *jointConfigurations[ i ].joint;
                const auto & jointInnerState = jointInnerStates[ i ];

                if ( jointConfiguration.positions().size() != 1 ) {
                    // TODO resolve by getting only user-configurable joints
                    continue;
                }
                assert( jointConfiguration.positions().size() == 1 );
                assert( jointConfiguration.jointLimits().size() == 1 );
                auto currentPosition = jointConfiguration.positions().front();
                auto jointLimits = jointConfiguration.jointLimits().front();

                auto [ posReached,
                       newPosition ] = jointInnerState.computeNewPosition( currentPosition,
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
        if ( auto [ ok, err ] = new_configuration->isValid( rofi::configuration::SimpleColision{} );
             !ok ) {
            throw std::runtime_error( err );
        }
        return std::pair( new_configuration, std::move( positionsReached ) );
    }

private:
    std::shared_ptr< const rofi::configuration::Rofibot > currentConfiguration() const
    {
        auto result = _physicalModulesConfiguration.visit( []( auto copy ) { return copy; } );
        assert( result );
        return result;
    }

    static std::map< ModuleId, ModuleInnerState > innerStatesFromConfiguration(
            const rofi::configuration::Rofibot & rofibotConfiguration );

private:
    atoms::Guarded< std::shared_ptr< const rofi::configuration::Rofibot > >
            _physicalModulesConfiguration;
    std::map< ModuleId, ModuleInnerState > _moduleInnerStates;

    atoms::Guarded< std::vector< std::shared_ptr< const rofi::configuration::Rofibot > > >
            _configurationHistory;
};

} // namespace rofi::simplesim
