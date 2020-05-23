#include "attacherPlugin.hpp"

#include <gazebo/common/Plugin.hh>
#include <ignition/math/Pose3.hh>

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

void AttacherPlugin::Load( physics::WorldPtr world, sdf::ElementPtr /*sdf*/ )
{
    _world = std::move( world );
    assert( _world );
    _physics = _world->Physics();
    assert( _physics );

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
}

bool AttacherPlugin::attach( std::pair< std::string, std::string > modelNames )
{
    modelNames = sortNames( std::move( modelNames ) );
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
        gzwarn << modelNames.first << " model was not found\n";
        return false;
    }
    if ( !model2 )
    {
        gzwarn << modelNames.second << " model was not found\n";
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
        gzwarn << "inner link of " << model1->GetScopedName() << " was not found\n";
        return false;
    }
    if ( !link2 )
    {
        gzwarn << "inner link of " << model2->GetScopedName() << " was not found\n";
        return false;
    }
    assert( link1 != link2 );


    std::lock_guard< std::mutex > lock( _connectedMutex );

    auto inserted = _connected.emplace( modelNames, LinkPair( link1, link2 ) );

    if ( !inserted.second )
    {
        gzwarn << "Already connected (" << modelNames.first << ", " << modelNames.second << ")\n";
        return false;
    }

    return true;
}

bool AttacherPlugin::detach( std::pair< std::string, std::string > modelNames )
{
    modelNames = sortNames( std::move( modelNames ) );

    if ( modelNames.first == modelNames.second )
    {
        return false;
    }
    assert( modelNames.first < modelNames.second );

    std::lock_guard< std::mutex > lock( _connectedMutex );

    auto it = _connected.find( modelNames );
    if ( it == _connected.end() )
    {
        return false;
    }

    auto jointPtr = std::get_if< physics::JointPtr >( &it->second );
    if ( jointPtr && *jointPtr )
    {
        auto joint = *jointPtr;
        joint->Detach();
    }

    _connected.erase( it );
    return true;
}

void AttacherPlugin::moveToOther( std::string thisRoficomName,
                                  std::string otherRoficomName,
                                  rofi::messages::ConnectorState::Orientation orientation )
{
    using rofi::messages::ConnectorState;

    if ( thisRoficomName == otherRoficomName )
    {
        return;
    }

    auto thisRoficom = _world->ModelByName( thisRoficomName );
    auto otherRoficom = _world->ModelByName( otherRoficomName );

    if ( !thisRoficom || !otherRoficom )
    {
        return;
    }

    auto thisRoficomLink = getLinkByName( thisRoficom, "inner" );
    auto otherRoficomLink = getLinkByName( otherRoficom, "inner" );

    if ( !thisRoficomLink || !otherRoficomLink )
    {
        return;
    }

    auto rotation = otherRoficom->WorldPose().Rot();
    switch ( orientation )
    {
        case ConnectorState::NORTH:
        {
            rotation = rotation * ignition::math::Quaterniond( 0, 1, 0, 0 );
            break;
        }
        case ConnectorState::EAST:
        {
            rotation = rotation * ignition::math::Quaterniond( 0, 0.707, 0.707, 0 );
            break;
        }
        case ConnectorState::SOUTH:
        {
            rotation = rotation * ignition::math::Quaterniond( 0, 0, 1, 0 );
            break;
        }
        case ConnectorState::WEST:
        {
            rotation = rotation * ignition::math::Quaterniond( 0, 0.707, -0.707, 0 );
            break;
        }
        default:
        {
            gzerr << "Unknown orientation\n";
            return;
        }
    }

    auto linkExtendedPos = thisRoficomLink->RelativePose().Pos();
    auto position = otherRoficomLink->WorldPose().Pos()
                    + otherRoficomLink->WorldPose().Rot().RotateVector( linkExtendedPos );

    thisRoficom->SetWorldPose( { position, rotation } );
}

void AttacherPlugin::sendAttachInfo( std::string modelname1,
                                     std::string modelname2,
                                     bool attach,
                                     rofi::messages::ConnectorState::Orientation orientation )
{
    sendAttachInfoToOne( modelname1, modelname2, attach, orientation );
    sendAttachInfoToOne( modelname2, modelname1, attach, orientation );
}

void AttacherPlugin::sendAttachInfoToOne( std::string modelname1,
                                          std::string modelname2,
                                          bool attach,
                                          rofi::messages::ConnectorState::Orientation orientation )
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

    if ( msg->attach() )
    {
        moveToOther( msg->modelname1(), msg->modelname2(), msg->orientation() );
        if ( attach( { msg->modelname1(), msg->modelname2() } ) )
        {
            gzmsg << "Attach was succesful\n";
            sendAttachInfo( msg->modelname1(), msg->modelname2(), true, msg->orientation() );
        }
        else
        {
            gzwarn << "Could not make the attach.\n";
        }
    }
    else
    {
        if ( detach( { msg->modelname1(), msg->modelname2() } ) )
        {
            gzmsg << "Detach was succesful\n";
            sendAttachInfo( msg->modelname1(), msg->modelname2(), false, msg->orientation() );
        }
        else
        {
            gzwarn << "Could not make the detach.\n";
        }
    }
}

void AttacherPlugin::onPhysicsUpdate()
{
    std::lock_guard< std::mutex > lock( _connectedMutex );

    for ( auto it = _connected.begin(); it != _connected.end(); it++ )
    {
        auto linksPtr = std::get_if< LinkPair >( &it->second );
        if ( !linksPtr )
        {
            continue;
        }

        auto & [ link1, link2 ] = *linksPtr;

        // apply force
        auto model1 = link1->GetModel();
        auto model2 = link2->GetModel();
        assert( model1 );
        assert( model2 );
        assert( model1 != model2 );

        auto pose1 = link1->WorldPose();
        auto pose2 = link2->WorldPose();

        auto diff = pose1 - pose2;
        double distance = diff.Pos().Length();

        if ( distance <= distancePrecision )
        {
            auto joint = createFixedJoint( _physics, { link1, link2 } );
            assert( joint );
            it->second = std::move( joint );
            continue;
        }

        // link2->SetLinearVel( link1->WorldLinearVel() );
        // link2->SetAngularVel( link1->WorldAngularVel() );

        ignition::math::Vector3d force = connectionForce / 2 * diff.Pos().Normalize();
        link1->AddForce( -force );
        link2->AddForce( force );
    }
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
