#include "attacherPlugin.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <optional>

#include "roficomConnect.hpp"
#include "roficomUtils.hpp"


namespace gazebo
{
std::pair< std::string, std::string > sortNames( std::pair< std::string, std::string > names )
{
    if ( names.first > names.second )
    {
        std::swap( names.first, names.second );
    }
    assert( names.first <= names.second );
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
    if ( lower == "3" || lower == "w" || lower == "west" )
    {
        return ConnectorState::WEST;
    }

    gzerr << "Unknown orientation '" + str + "'\n";
    throw std::runtime_error( "Unknown orientation '" + str + "'" );
}

ignition::math::Quaterniond rotation( rofi::messages::ConnectorState::Orientation orientation )
{
    using rofi::messages::ConnectorState;
    using namespace ignition::math;

    switch ( orientation )
    {
        case ConnectorState::NORTH:
        {
            return Quaterniond::EulerToQuaternion( 0, 0, 0 );
        }
        case ConnectorState::EAST:
        {
            return Quaterniond::EulerToQuaternion( 0, 0, -Angle::HalfPi() );
        }
        case ConnectorState::SOUTH:
        {
            return Quaterniond::EulerToQuaternion( 0, 0, Angle::Pi() );
        }
        case ConnectorState::WEST:
        {
            return Quaterniond::EulerToQuaternion( 0, 0, Angle::HalfPi() );
        }
        default:
        {
            gzerr << "Unknown orientation for rotation\n";
            return {};
        }
    }
}

void AttacherPlugin::Load( physics::WorldPtr world, sdf::ElementPtr sdf )
{
    _world = std::move( world );
    assert( _world );
    _physics = _world->Physics();
    assert( _physics );
    _sdf = std::move( sdf );
    assert( _sdf );

    loadConnectionsFromSdf();

    _onUpdate = event::Events::ConnectBeforePhysicsUpdate(
            std::bind( &AttacherPlugin::onPhysicsUpdate, this ) );

    _node = boost::make_shared< transport::Node >();
    assert( _node );
    _node->Init( _world->Name() );
    assert( _node && _node->IsInitialized() );

    _subAttachEvent =
            _node->Subscribe( "~/attach_event", &AttacherPlugin::attach_event_callback, this );
    assert( _subAttachEvent );
    gzmsg << "Attach event service at: " << _subAttachEvent->GetTopic() << "\n";

    gzmsg << "Link attacher node initialized.\n";

    onPhysicsUpdate();
}

void AttacherPlugin::loadConnectionsFromSdf()
{
    assert( _sdf );

    checkChildrenNames( _sdf, { "connection" } );
    for ( auto connectionSdf : getChildren( _sdf, "connection" ) )
    {
        checkChildrenNames( connectionSdf, { "roficom", "orientation" } );

        auto roficoms = getChildren( connectionSdf, "roficom" );
        if ( roficoms.size() != 2 )
        {
            std::cerr << "Expected exactly two roficoms in connection\n";
            continue;
        }
        auto roficomNames = sortNames(
                { roficoms[ 0 ]->Get< std::string >(), roficoms[ 1 ]->Get< std::string >() } );

        auto orientationSdf = getOnlyChild< false >( connectionSdf, "orientation" );
        std::optional< Orientation > orientation;
        if ( orientationSdf )
        {
            orientation = readOrientation( orientationSdf->Get< std::string >() );
        }

        attach( roficomNames, orientation );
    }
}

void AttacherPlugin::addConnectionToSdf( StringPair modelNames,
                                         std::optional< Orientation > orientation )
{
    assert( _sdf );

    auto connectionSdf = newElement( "connection" );
    insertElement( _sdf, connectionSdf );

    insertElement( connectionSdf, newElemWithValue( "roficom", modelNames.first ) );
    insertElement( connectionSdf, newElemWithValue( "roficom", modelNames.second ) );

    if ( orientation )
    {
        insertElement( connectionSdf, newElemWithValue( "orientation", *orientation ) );
    }
}

void AttacherPlugin::removeConnectionFromSdf( StringPair modelNames )
{
    assert( _sdf );
    modelNames = sortNames( std::move( modelNames ) );

    for ( auto connectionSdf : getChildren( _sdf, "connection" ) )
    {
        auto roficoms = getChildren( connectionSdf, "roficom" );
        if ( roficoms.size() != 2 )
        {
            continue;
        }

        if ( sortNames(
                     { roficoms[ 0 ]->Get< std::string >(), roficoms[ 1 ]->Get< std::string >() } )
             == modelNames )
        {
            connectionSdf->RemoveFromParent();
        }
    }
}

bool AttacherPlugin::attach( StringPair modelNames, std::optional< Orientation > orientation )
{
    modelNames = sortNames( std::move( modelNames ) );
    if ( modelNames.first.empty() || modelNames.second.empty() )
    {
        gzwarn << "Got empty model name\n";
        return false;
    }
    if ( modelNames.first == modelNames.second )
    {
        gzwarn << "Same model name " << modelNames.first << "\n";
        return false;
    }
    assert( modelNames.first < modelNames.second );

    gzmsg << "Attaching roficoms\n";

    auto model1 = _world->ModelByName( modelNames.first );
    auto model2 = _world->ModelByName( modelNames.second );

    if ( !model1 )
    {
        gzwarn << "Model '" << modelNames.first << "' was not found\n";
        return false;
    }
    if ( !model2 )
    {
        gzwarn << "Model '" << modelNames.second << "' was not found\n";
        return false;
    }
    if ( model1 == model2 )
    {
        gzwarn << "Found the same model\n";
        return false;
    }

    auto link1 = getLinkByName( model1, "inner" );
    auto link2 = getLinkByName( model2, "inner" );

    if ( !link1 )
    {
        gzwarn << "Link 'inner' of " << model1->GetScopedName() << " was not found\n";
        return false;
    }
    if ( !link2 )
    {
        gzwarn << "Link 'inner' of " << model2->GetScopedName() << " was not found\n";
        return false;
    }
    assert( link1 != link2 );


    auto connected = canRoficomBeConnected( link1->WorldPose(), link2->WorldPose() );

    if ( !connected )
    {
        gzwarn << "Could not get connection orientation\n";
        return false;
    }

    assert( connected );
    if ( orientation && connected != orientation )
    {
        gzwarn << "Got message with different orientation\n";
    }

    std::lock_guard< std::mutex > lock( _connectedMutex );

    auto inserted =
            _connected.emplace( modelNames, ConnectedInfo( *connected, LinkPair( link1, link2 ) ) );

    if ( !inserted.second )
    {
        gzwarn << "Already connected (" << modelNames.first << ", " << modelNames.second << ")\n";
        return false;
    }

    return true;
}

std::optional< AttacherPlugin::Orientation > AttacherPlugin::detach(
        std::pair< std::string, std::string > modelNames )
{
    modelNames = sortNames( std::move( modelNames ) );

    if ( modelNames.first == modelNames.second )
    {
        return {};
    }
    assert( modelNames.first < modelNames.second );

    std::lock_guard< std::mutex > lock( _connectedMutex );

    auto it = _connected.find( modelNames );
    if ( it == _connected.end() )
    {
        return {};
    }

    auto jointPtr = std::get_if< physics::JointPtr >( &it->second.info );
    if ( jointPtr && *jointPtr )
    {
        auto joint = *jointPtr;
        joint->Detach();
    }

    std::optional< Orientation > orientation = it->second.orientation;
    _connected.erase( it );
    return orientation;
}

void AttacherPlugin::sendAttachInfo( std::string modelname1,
                                     std::string modelname2,
                                     bool attach,
                                     Orientation orientation )
{
    sendAttachInfoToOne( modelname1, modelname2, attach, orientation );
    sendAttachInfoToOne( modelname2, modelname1, attach, orientation );
}

void AttacherPlugin::sendAttachInfoToOne( std::string modelname1,
                                          std::string modelname2,
                                          bool attach,
                                          Orientation orientation )
{
    rofi::messages::ConnectorAttachInfo msg;
    msg.set_modelname1( std::move( modelname1 ) );
    msg.set_modelname2( std::move( modelname2 ) );
    msg.set_attach( attach );
    msg.set_orientation( orientation );

    auto model1 = _world->ModelByName( msg.modelname1() );
    assert( model1 );

    auto path = "/gazebo/" + getElemPath( model1 ) + "/attach_event";
    _node->Publish< rofi::messages::ConnectorAttachInfo >( path, msg );
}

void AttacherPlugin::attach_event_callback( const AttacherPlugin::ConnectorAttachInfoPtr & msg )
{
    gzmsg << "Received request to " << ( msg->attach() ? "attach" : "detach" ) << " '"
          << msg->modelname1() << "' with '" << msg->modelname2() << "'\n";

    if ( msg->modelname1().empty() || msg->modelname2().empty() )
    {
        gzwarn << "One of models to attach/detach is empty\n";
        return;
    }
    if ( msg->modelname1() == msg->modelname2() )
    {
        gzwarn << "Both models are the same\n";
        return;
    }

    if ( !rofi::messages::ConnectorState::Orientation_IsValid( msg->orientation() ) )
    {
        std::cerr << "Attacher got message with invalid orientation (" << msg->orientation()
                  << ")\n";
        return;
    }

    if ( msg->attach() )
    {
        if ( attach( { msg->modelname1(), msg->modelname2() }, msg->orientation() ) )
        {
            addConnectionToSdf( { msg->modelname1(), msg->modelname2() }, msg->orientation() );
            gzmsg << "Attach was succesful\n";
        }
        else
        {
            gzwarn << "Could not make the attach.\n";
        }
    }
    else
    {
        auto orientation = detach( { msg->modelname1(), msg->modelname2() } );
        if ( orientation )
        {
            gzmsg << "Detach was succesful\n";
            removeConnectionFromSdf( { msg->modelname1(), msg->modelname2() } );
            sendAttachInfo( msg->modelname1(), msg->modelname2(), false, *orientation );
        }
        else
        {
            gzwarn << "Could not make the detach.\n";
        }
    }
}

void AttacherPlugin::onPhysicsUpdate()
{
    using rofi::messages::ConnectorState;

    std::lock_guard< std::mutex > lock( _connectedMutex );

    for ( auto it = _connected.begin(); it != _connected.end(); it++ )
    {
        auto linksPtr = std::get_if< LinkPair >( &it->second.info );
        if ( !linksPtr )
        {
            continue;
        }
        auto & links = *linksPtr;

        bool connected = applyAttractForce( links, it->second.orientation );
        if ( connected )
        {
            gzmsg << "Creating joint\n";
            auto joint = createFixedJoint( _physics, links );
            assert( joint );
            it->second.info = std::move( joint );
            auto & names = it->first;
            sendAttachInfo( names.first, names.second, true, it->second.orientation );
        }
    }
}

bool AttacherPlugin::applyAttractForce( LinkPair links, Orientation orientation )
{
    using rofi::messages::ConnectorState;
    using namespace ignition::math;

    assert( links.first );
    assert( links.second );
    assert( links.first != links.second );

    auto pose1 = links.first->WorldPose();
    auto pose2 = Pose3d( {}, { Angle::Pi(), 0, 0 } )
                 + Pose3d( {}, rotation( orientation ).Inverse() ) + links.second->WorldPose();

    auto firstToSecond = pose1.CoordPoseSolve( pose2 );


    static_assert( ConnectorState::NORTH == static_cast< Orientation >( 0 ) );
    static_assert( ConnectorState::EAST == static_cast< Orientation >( 1 ) );
    static_assert( ConnectorState::SOUTH == static_cast< Orientation >( 2 ) );
    static_assert( ConnectorState::WEST == static_cast< Orientation >( 3 ) );

    constexpr int pointsCount = 4;

    std::array< Vector3d, pointsCount > firstPoints;
    std::array< Vector3d, pointsCount > secondPoints;
    std::array< Vector3d, pointsCount > diffs;

    for ( size_t i = 0; i < pointsCount; i++ )
    {
        auto side = static_cast< Orientation >( i );

        firstPoints.at( i ) = rotation( side ).RotateVector( { 10e-3, 0, 0 } );
        secondPoints.at( i ) = firstToSecond.CoordPositionAdd( firstPoints.at( i ) );

        diffs.at( i ) = secondPoints.at( i ) - firstPoints.at( i );
    }

    if ( std::all_of( diffs.begin(), diffs.end(), []( auto diff ) {
             return diff.Length() <= distancePrecision;
         } ) )
    {
        return true;
    }

    for ( size_t i = 0; i < pointsCount; i++ )
    {
        auto force = connectionForce / 8 * diffs.at( i ).Normalized();
        links.first->AddForceAtRelativePosition( force, firstPoints.at( i ) );
        links.second->AddForceAtRelativePosition( -force, secondPoints.at( i ) );
    }
    return false;
}


physics::JointPtr AttacherPlugin::createFixedJoint( physics::PhysicsEnginePtr physics,
                                                    LinkPair links )
{
    assert( links.first );
    assert( links.second );
    assert( links.first != links.second );
    assert( links.first->GetModel() != links.second->GetModel() );

    auto joint = physics->CreateJoint( "revolute", links.first->GetModel() );

    joint->Attach( links.first, links.second );
    joint->Load( links.first, links.second, ignition::math::Pose3d() );
    joint->SetModel( links.first->GetModel() );
    joint->SetUpperLimit( 0, 0 );
    joint->SetLowerLimit( 0, 0 );
    joint->Init();

    gzmsg << "Joint created (" << links.first->GetName() << ", " << links.second->GetName()
          << ")\n";

    return joint;
}

GZ_REGISTER_WORLD_PLUGIN( AttacherPlugin )

} // namespace gazebo
