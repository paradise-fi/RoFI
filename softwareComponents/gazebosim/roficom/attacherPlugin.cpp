#include "attacherPlugin.hpp"

#include <algorithm>
#include <cctype>

#include <gz/plugin/Register.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/World.hh>
#include <gz/sim/components/ChildLinkName.hh>
#include <gz/sim/components/DetachableJoint.hh>
#include <gz/sim/components/Joint.hh>
#include <gz/sim/components/JointType.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/ParentLinkName.hh>

namespace gazebo
{
std::pair< std::string, std::string > sortNames( std::pair< std::string, std::string > names )
{
    if ( names.first > names.second )
    {
        std::swap( names.first, names.second );
    }
    return names;
}

rofi::messages::ConnectorState::Orientation readOrientation( const std::string & str )
{
    using rofi::messages::ConnectorState;

    std::string lower;
    for ( char c : str )
    {
        lower.push_back( static_cast< char >( std::tolower( static_cast< unsigned char >( c ) ) ) );
    }

    if ( lower == "0" || lower == "n" || lower == "north" )
    {
        return ConnectorState::NORTH;
    }
    if ( lower == "1" || lower == "e" || lower == "east" )
    {
        return ConnectorState::EAST;
    }
    if ( lower == "2" || lower == "s" || lower == "south" )
    {
        return ConnectorState::SOUTH;
    }
    return ConnectorState::WEST;
}

gz::math::Quaterniond rotation( rofi::messages::ConnectorState::Orientation orientation )
{
    using rofi::messages::ConnectorState;
    using gz::math::Angle;
    using gz::math::Quaterniond;

    switch ( orientation )
    {
        case ConnectorState::NORTH:
            return Quaterniond::EulerToQuaternion( 0, 0, 0 );
        case ConnectorState::EAST:
            return Quaterniond::EulerToQuaternion( 0, 0, -Angle::HalfPi() );
        case ConnectorState::SOUTH:
            return Quaterniond::EulerToQuaternion( 0, 0, Angle::Pi() );
        case ConnectorState::WEST:
            return Quaterniond::EulerToQuaternion( 0, 0, Angle::HalfPi() );
        default:
            return {};
    }
}

void AttacherPlugin::Configure( const gz::sim::Entity & entity,
                                const std::shared_ptr< const sdf::Element > & sdf,
                                gz::sim::EntityComponentManager & ecm,
                                gz::sim::EventManager & )
{
    _worldEntity = entity;
    _sdf = sdf;
    _worldName = gz::sim::World( entity ).Name( ecm ).value_or( "default" );

    _node = std::make_shared< rofi::gz::Node >();
    _node->Init( _worldName );
    _subAttachEvent = _node->Subscribe( "~/attach_event", &AttacherPlugin::attach_event_callback, this );

    loadConnectionsFromSdf();
}

void AttacherPlugin::PreUpdate( const gz::sim::UpdateInfo &,
                                gz::sim::EntityComponentManager & ecm )
{
    std::vector< rofi::messages::ConnectorAttachInfo > pending;
    {
        std::lock_guard lock( _queueMutex );
        pending.swap( _pendingAttachRequests );
    }

    for ( const auto & msg : pending )
    {
        if ( msg.attach() )
        {
            attach( { msg.modelname1(), msg.modelname2() }, msg.orientation(), ecm );
        }
        else if ( auto orientation = detach( { msg.modelname1(), msg.modelname2() }, ecm ) )
        {
            sendAttachInfo( msg.modelname1(), msg.modelname2(), false, *orientation );
        }
    }
}

void AttacherPlugin::loadConnectionsFromSdf()
{
    auto sdf = std::const_pointer_cast< sdf::Element >( _sdf );
    checkChildrenNames( sdf, { "connection" } );
    for ( auto connectionSdf : getChildren( sdf, "connection" ) )
    {
        auto roficoms = getChildren( connectionSdf, "roficom" );
        if ( roficoms.size() != 2 )
        {
            continue;
        }

        rofi::messages::ConnectorAttachInfo msg;
        msg.set_modelname1( roficoms[ 0 ]->Get< std::string >() );
        msg.set_modelname2( roficoms[ 1 ]->Get< std::string >() );
        msg.set_attach( true );
        if ( auto orientation = getOnlyChild< false >( connectionSdf, "orientation" ) )
        {
            msg.set_orientation( readOrientation( orientation->Get< std::string >() ) );
        }

        std::lock_guard lock( _queueMutex );
        _pendingAttachRequests.push_back( msg );
    }
}

bool AttacherPlugin::attach( StringPair modelNames,
                             std::optional< Orientation > orientation,
                             gz::sim::EntityComponentManager & ecm )
{
    modelNames = sortNames( std::move( modelNames ) );
    if ( modelNames.first == modelNames.second || modelNames.first.empty()
         || modelNames.second.empty() )
    {
        return false;
    }

    auto model1 = findModelEntityByScopedName( modelNames.first, ecm, _worldEntity );
    auto model2 = findModelEntityByScopedName( modelNames.second, ecm, _worldEntity );
    if ( !model1 || !model2 || !isRoFICoM( *model1, ecm ) || !isRoFICoM( *model2, ecm ) )
    {
        return false;
    }

    auto link1 = getLinkByName( *model1, ecm, "inner" );
    auto link2 = getLinkByName( *model2, ecm, "inner" );
    if ( !link1 || !link2 )
    {
        return false;
    }

    auto pose1 = gz::sim::Link( *link1 ).WorldPose( ecm );
    auto pose2 = gz::sim::Link( *link2 ).WorldPose( ecm );
    if ( !pose1 || !pose2 )
    {
        return false;
    }

    auto connected = canRoficomBeConnected( *pose1, *pose2 );
    if ( !connected )
    {
        return false;
    }
    if ( orientation && *orientation != *connected )
    {
        gzwarn << "Attach request orientation differs from detected orientation\n";
    }

    std::lock_guard lock( _connectedMutex );
    if ( _connected.contains( modelNames ) )
    {
        return false;
    }

    auto jointName =
            "rofi_attacher_" + std::to_string( static_cast< uint64_t >( *link1 ) ) + "_"
            + std::to_string( static_cast< uint64_t >( *link2 ) );
    auto jointEntity = createFixedJoint( *link1, *link2, jointName, ecm );
    _connected.emplace( modelNames, ConnectedInfo{ *connected, jointEntity } );
    sendAttachInfo( modelNames.first, modelNames.second, true, *connected );
    return true;
}

std::optional< AttacherPlugin::Orientation > AttacherPlugin::detach(
        StringPair modelNames,
        gz::sim::EntityComponentManager & ecm )
{
    modelNames = sortNames( std::move( modelNames ) );

    std::lock_guard lock( _connectedMutex );
    auto it = _connected.find( modelNames );
    if ( it == _connected.end() )
    {
        return {};
    }

    if ( it->second.jointEntity != gz::sim::kNullEntity )
    {
        ecm.RequestRemoveEntity( it->second.jointEntity, true );
    }

    auto orientation = it->second.orientation;
    _connected.erase( it );
    return orientation;
}

void AttacherPlugin::sendAttachInfo( const std::string & modelName1,
                                     const std::string & modelName2,
                                     bool attach,
                                     Orientation orientation )
{
    sendAttachInfoToOne( modelName1, modelName2, attach, orientation );
    sendAttachInfoToOne( modelName2, modelName1, attach, orientation );
}

void AttacherPlugin::sendAttachInfoToOne( const std::string & roficomName,
                                          const std::string & connectedToName,
                                          bool attach,
                                          Orientation orientation )
{
    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( roficomName );
    msg.set_modelname2( connectedToName );
    msg.set_attach( attach );
    msg.set_orientation( orientation );
    _node->Advertise< rofi::messages::ConnectorAttachInfo >( "/gazebo/"
                                                             + scopedNameToPath( roficomName )
                                                             + "/attach_event" )
            ->Publish( msg, true );
}

void AttacherPlugin::attach_event_callback( const ConnectorAttachInfoPtr & msg )
{
    if ( !msg )
    {
        return;
    }
    std::lock_guard lock( _queueMutex );
    _pendingAttachRequests.push_back( *msg );
}

gz::sim::Entity AttacherPlugin::createFixedJoint( gz::sim::Entity parentLink,
                                                  gz::sim::Entity childLink,
                                                  const std::string & name,
                                                  gz::sim::EntityComponentManager & ecm )
{
    auto jointEntity = ecm.CreateEntity();
    ecm.SetParentEntity( jointEntity, childLink );
    ecm.CreateComponent( jointEntity, gz::sim::components::Joint() );
    ecm.CreateComponent( jointEntity, gz::sim::components::Name( name ) );
    ecm.CreateComponent( jointEntity, gz::sim::components::JointType( sdf::JointType::FIXED ) );
    ecm.CreateComponent( jointEntity,
                         gz::sim::components::DetachableJoint(
                                 { parentLink, childLink, "fixed" } ) );

    auto parentName = gz::sim::Link( parentLink ).Name( ecm ).value_or( "" );
    auto childName = gz::sim::Link( childLink ).Name( ecm ).value_or( "" );
    ecm.CreateComponent( jointEntity, gz::sim::components::ParentLinkName( parentName ) );
    ecm.CreateComponent( jointEntity, gz::sim::components::ChildLinkName( childName ) );

    return jointEntity;
}

} // namespace gazebo

GZ_ADD_PLUGIN( gazebo::AttacherPlugin,
               gz::sim::System,
               gz::sim::ISystemConfigure,
               gz::sim::ISystemPreUpdate )
GZ_ADD_PLUGIN_ALIAS( gazebo::AttacherPlugin, "rofi::sim::systems::AttacherPlugin" )
