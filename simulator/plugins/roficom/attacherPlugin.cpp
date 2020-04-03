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
        gzerr << "Same model name " << modelNames.first << "\n";
        return false;
    }

    assert( modelNames.first < modelNames.second );

    gzmsg << "Creating new joint.\n";

    auto it = _joints.find( modelNames );
    if ( it != _joints.end() )
    {
        gzwarn << "Already connected (" << modelNames.first << ", " << modelNames.second << ")\n";
        return false;
    }

    auto model1 = _world->ModelByName( modelNames.first );
    auto model2 = _world->ModelByName( modelNames.second );

    if ( !model1 )
    {
        gzerr << modelNames.first << " model was not found\n";
        return false;
    }
    if ( !model2 )
    {
        gzerr << modelNames.second << " model was not found\n";
        return false;
    }
    if ( model1 == model2 )
    {
        gzerr << "Found the same model\n";
        return false;
    }

    auto link1 = getLinkByName( model1, "inner" );
    auto link2 = getLinkByName( model2, "inner" );

    if ( !link1 )
    {
        gzerr << "inner link of " << model1->GetScopedName() << " was not found\n";
        return false;
    }
    if ( !link2 )
    {
        gzerr << "inner link of " << model2->GetScopedName() << " was not found\n";
        return false;
    }

    assert( model1 );
    assert( model2 );
    assert( link1 );
    assert( link2 );
    assert( model1 != model2 );
    assert( link1 != link2 );

    auto joint = _physics->CreateJoint( "revolute", model1 );
    _joints.emplace( modelNames, joint );

    joint->Attach( link1, link2 );
    joint->Load( link1, link2, ignition::math::Pose3d() );
    joint->SetModel( model1 );
    joint->SetUpperLimit( 0, 0 );
    joint->SetLowerLimit( 0, 0 );
    joint->Init();

    gzmsg << "Joint created.\n";

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

    auto it = _joints.find( modelNames );
    if ( it != _joints.end() )
    {
        it->second->Detach();
        _joints.erase( it );
        return true;
    }

    return false;
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
    msg.set_attach( true );
    msg.set_orientation( orientation );

    auto model1 = _world->ModelByName( msg.modelname1() );
    assert( model1 );

    auto path = "/gazebo/" + getElemPath( model1 ) + "/attach_event";
    gzmsg << "Publishing attach info on path '" << path << "'\n";
    _node->Publish( path, msg );
}

void AttacherPlugin::attach_event_callback( const AttacherPlugin::ConnectorAttachInfoPtr & msg )
{
    gzmsg << "Received request to " << ( msg->attach() ? "attach" : "detach" ) << " '"
          << msg->modelname1() << "' with '" << msg->modelname2() << "'\n";

    if ( msg->modelname1().empty() || msg->modelname2().empty() )
    {
        gzerr << "One of models to attach/detach is empty\n";
        return;
    }
    if ( msg->modelname1() == msg->modelname2() )
    {
        gzerr << "Both models are the same\n";
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
            gzerr << "Could not make the attach.\n";
        }
    }
    else
    {
        if ( !detach( { msg->modelname1(), msg->modelname2() } ) )
        {
            gzerr << "Could not make the detach.\n";
        }
        else
        {
            gzmsg << "Detach was succesful\n";
            sendAttachInfo( msg->modelname1(), msg->modelname2(), false, msg->orientation() );
        }
    }
}

GZ_REGISTER_WORLD_PLUGIN( AttacherPlugin )

} // namespace gazebo
