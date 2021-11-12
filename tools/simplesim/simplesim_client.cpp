#include "simplesim_client.hpp"

#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOBJReader.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

#include "atoms/concurrent_queue.hpp"
#include "atoms/guarded.hpp"
#include "atoms/resources.hpp"


using rofi::simplesim::SimplesimClient;
using rofi::simplesim::detail::ModuleRenderInfo;

using namespace rofi::configuration::matrices;

vtkSmartPointer< vtkMatrix4x4 > convertMatrix( const Matrix & m )
{
    auto mat = vtkSmartPointer< vtkMatrix4x4 >::New();
    for ( int i = 0; i < 4; i++ ) {
        for ( int j = 0; j < 4; j++ ) {
            mat->SetElement( i, j, m( i, j ) );
        }
    }
    return mat;
}

constexpr std::array< double, 3 > getModuleColor( int moduleId )
{
    const std::array< std::array< uint8_t, 3 >, 7 > palette = { { { 191, 218, 112 },
                                                                  { 242, 202, 121 },
                                                                  { 218, 152, 207 },
                                                                  { 142, 202, 222 },
                                                                  { 104, 135, 205 },
                                                                  { 250, 176, 162 },
                                                                  { 234, 110, 111 } } };
    auto color = palette[ moduleId % palette.size() ];
    return { color[ 0 ] / 255.0, color[ 1 ] / 255.0, color[ 2 ] / 255.0 };
}

vtkAlgorithmOutput * getComponentModel( rofi::configuration::ComponentType type )
{
    using ComponentType = rofi::configuration::ComponentType;

    static const std::map< ComponentType, std::function< ResourceFile() > > resourceMap(
            { { ComponentType::UmShoe, LOAD_RESOURCE_FILE_LAZY( model_shoe_obj ) },
              { ComponentType::UmBody, LOAD_RESOURCE_FILE_LAZY( model_body_obj ) },
              { ComponentType::Roficom, LOAD_RESOURCE_FILE_LAZY( model_connector_obj ) } } );
    static std::map< ComponentType, vtkSmartPointer< vtkTransformPolyDataFilter > > cache;

    assert( resourceMap.count( type ) == 1 && "Unsupported component type specified" );

    if ( cache.count( type ) == 0 ) {
        auto reader = vtkSmartPointer< vtkOBJReader >::New();
        ResourceFile modelFile = resourceMap.find( type )->second();
        reader->SetFileName( modelFile.name().c_str() );
        reader->Update();

        auto trans = vtkSmartPointer< vtkTransform >::New();
        trans->RotateX( 90 );
        auto t = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
        t->SetInputConnection( reader->GetOutputPort() );
        t->SetTransform( trans );
        t->Update();

        cache.insert( { type, t } );
    }
    return cache[ type ]->GetOutputPort();
}


std::vector< vtkSmartPointer< vtkActor > > addModuleToScene(
        vtkRenderer * renderer,
        const rofi::configuration::Module & mod,
        const Matrix & mPosition,
        std::array< double, 3 > moduleColor,
        const std::unordered_set< int > & activeConnectors )
{
    auto componentActors = std::vector< vtkSmartPointer< vtkActor > >();
    componentActors.reserve( mod.components().size() );
    const auto & components = mod.components();
    for ( int i = 0; i != components.size(); i++ ) {
        const auto & component = components[ i ];
        auto cPosition = mod.getComponentPosition( i, mPosition );

        // make connected RoFICoMs connected visually
        if ( activeConnectors.count( i ) > 0 ) {
            cPosition = cPosition * translate( { -0.05, 0, 0 } );
        }

        auto posTrans = vtkSmartPointer< vtkTransform >::New();
        posTrans->SetMatrix( convertMatrix( cPosition ) );

        auto filter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
        filter->SetTransform( posTrans );
        filter->SetInputConnection( getComponentModel( component.type ) );

        auto frameMapper = vtkSmartPointer< vtkPolyDataMapper >::New();
        frameMapper->SetInputConnection( filter->GetOutputPort() );

        auto frameActor = vtkSmartPointer< vtkActor >::New();
        frameActor->SetMapper( frameMapper );
        frameActor->GetProperty()->SetColor( moduleColor.data() );
        frameActor->GetProperty()->SetOpacity( 1.0 );
        frameActor->GetProperty()->SetFrontfaceCulling( true );
        frameActor->GetProperty()->SetBackfaceCulling( true );
        frameActor->SetPosition( cPosition( 0, 3 ), cPosition( 1, 3 ), cPosition( 2, 3 ) );
        frameActor->SetScale( 1 / 95.0 );

        renderer->AddActor( frameActor );
        componentActors.push_back( std::move( frameActor ) );
    }
    return componentActors;
}

void removeModuleFromScene( vtkRenderer * renderer,
                            const std::vector< vtkSmartPointer< vtkActor > > & componentActors )
{
    for ( auto & componentActor : componentActors ) {
        renderer->RemoveActor( componentActor );
    }
}


std::map< rofi::configuration::ModuleId, ModuleRenderInfo > addConfigurationToRenderer(
        vtkRenderer * renderer,
        const rofi::configuration::Rofibot & newConfiguration )
{
    assert( renderer != nullptr );
    std::map< rofi::configuration::ModuleId, ModuleRenderInfo > moduleRenderInfos;
    // get active (i.e. connected) connectors for each module within Rofibot
    // TODO get from the inner state
    for ( const auto & roficomConnection : newConfiguration.roficoms() ) {
        moduleRenderInfos[ roficomConnection.sourceModule ].activeConnectors.insert(
                roficomConnection.sourceConnector );
        moduleRenderInfos[ roficomConnection.destModule ].activeConnectors.insert(
                roficomConnection.destConnector );
    }

    for ( const auto & moduleInfo : newConfiguration.modules() ) {
        assert( moduleInfo.module.get() );
        assert( moduleInfo.position && "The configuration has to be prepared" );
        addModuleToScene( renderer,
                          *moduleInfo.module,
                          *moduleInfo.position,
                          getModuleColor( moduleInfo.module->getId() ),
                          moduleRenderInfos[ moduleInfo.module->getId() ].activeConnectors );
    }

    assert( moduleRenderInfos.size() == newConfiguration.modules().size() );
    return moduleRenderInfos;
}


SimplesimClient::SimplesimClient()
{
    _renderer->SetBackground( 1.0, 1.0, 1.0 );
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom( 1.5 );


    _renderWindow->SetSize( 600, 400 );
    _renderWindow->SetWindowName( "RoFI simulation" );
    _renderWindow->AddRenderer( _renderer.Get() );


    _renderWindowInteractor->SetRenderWindow( _renderWindow.Get() );
    _renderWindowInteractor->SetInteractorStyle( _interactorStyle.Get() );

    _renderWindowInteractor->Initialize();

    _updateConfigurationCommand->client = this;
    _renderWindowInteractor->CreateRepeatingTimer( 1 );
    _renderWindowInteractor->AddObserver( vtkCommand::TimerEvent,
                                          _updateConfigurationCommand.Get() );
}

void SimplesimClient::clearRenderer()
{
    for ( auto & moduleRenderInfo : _moduleRenderInfos ) {
        removeModuleFromScene( _renderer.Get(), moduleRenderInfo.second.componentActors );
    }
    _moduleRenderInfos.clear();
    _lastRenderedConfiguration.reset();
}

void SimplesimClient::renderCurrentConfiguration()
{
    // TODO do not render every frame from the begining
    assert( _renderer.Get() != nullptr );
    if ( auto newConfiguration = getCurrentConfig() ) {
        clearRenderer();
        assert( _moduleRenderInfos.empty() );
        _moduleRenderInfos = addConfigurationToRenderer( _renderer.Get(), *newConfiguration );

        _lastRenderedConfiguration = std::move( newConfiguration );
        _renderWindow->Render();
    }
}
