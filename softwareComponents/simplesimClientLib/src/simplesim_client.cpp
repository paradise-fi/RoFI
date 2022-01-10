#include "simplesim_client.hpp"

#include <array>
#include <functional>
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
    constexpr std::array< std::array< uint8_t, 3 >, 7 > palette = { { { 191, 218, 112 },
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

vtkSmartPointer< vtkPolyDataMapper > getComponentMapper( const Matrix & cPosition,
                                                         rofi::configuration::ComponentType cType )
{
    auto posTransform = vtkSmartPointer< vtkTransform >::New();
    posTransform->SetMatrix( convertMatrix( cPosition ) );

    auto filter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    filter->SetTransform( posTransform );
    filter->SetInputConnection( getComponentModel( cType ) );

    auto mapper = vtkSmartPointer< vtkPolyDataMapper >::New();
    mapper->SetInputConnection( filter->GetOutputPort() );
    return mapper;
}


[[nodiscard]] std::vector< vtkSmartPointer< vtkActor > > addModuleToScene(
        vtkRenderer * renderer,
        const rofi::configuration::Module & newModule,
        const Matrix & mPosition,
        const std::unordered_set< int > & activeConnectors )
{
    auto componentActors = std::vector< vtkSmartPointer< vtkActor > >();
    componentActors.reserve( newModule.components().size() );
    const auto & components = newModule.components();
    for ( int i = 0; i != components.size(); i++ ) {
        auto frameActor = vtkSmartPointer< vtkActor >::New();
        frameActor->GetProperty()->SetColor( getModuleColor( newModule.getId() ).data() );
        frameActor->GetProperty()->SetOpacity( 1.0 );
        frameActor->GetProperty()->SetFrontfaceCulling( true );
        frameActor->GetProperty()->SetBackfaceCulling( true );
        frameActor->SetScale( 1 / 95.0 );

        auto cPosition = newModule.getComponentPosition( i, mPosition );
        // make connected RoFICoMs connected visually
        if ( activeConnectors.count( i ) > 0 ) {
            cPosition = cPosition * translate( { -0.05, 0, 0 } );
        }

        auto mapper = getComponentMapper( cPosition, components[ i ].type );
        frameActor->SetMapper( mapper );
        frameActor->SetPosition( cPosition( 0, 3 ), cPosition( 1, 3 ), cPosition( 2, 3 ) );

        renderer->AddActor( frameActor );
        componentActors.push_back( std::move( frameActor ) );
    }
    assert( componentActors.size() == newModule.components().size() );
    return componentActors;
}
bool sameComponentTypes( const rofi::configuration::Module & newModule,
                         const rofi::configuration::Module & previousModule )
{
    const auto & newComponents = newModule.components();
    const auto & previousComponents = previousModule.components();

    if ( newComponents.size() != previousComponents.size() ) {
        return false;
    }

    for ( size_t i = 0; i < newComponents.size(); i++ ) {
        if ( newComponents[ i ].type != previousComponents[ i ].type ) {
            return false;
        }
    }
    return true;
}
void removeModuleFromScene( vtkRenderer * renderer,
                            std::vector< vtkSmartPointer< vtkActor > > & componentActors )
{
    for ( const auto & componentActor : componentActors ) {
        renderer->RemoveActor( componentActor );
    }
    componentActors.clear();
}
void updateModulePositionInScene( const rofi::configuration::Module & newModule,
                                  const Matrix & mPosition,
                                  const ModuleRenderInfo & moduleRenderInfo )
{
    const auto & components = newModule.components();
    assert( moduleRenderInfo.componentActors.size() == components.size() );

    for ( int i = 0; i != components.size(); i++ ) {
        const auto & componentActor = moduleRenderInfo.componentActors[ i ];

        auto cPosition = newModule.getComponentPosition( i, mPosition );
        // make connected RoFICoMs connected visually
        if ( moduleRenderInfo.activeConnectors.count( i ) > 0 ) {
            cPosition = cPosition * translate( { -0.05, 0, 0 } );
        }

        auto mapper = getComponentMapper( cPosition, components[ i ].type );

        componentActor->SetMapper( mapper );
        componentActor->SetPosition( cPosition( 0, 3 ), cPosition( 1, 3 ), cPosition( 2, 3 ) );
    }
}
void updateModuleInScene( vtkRenderer * renderer,
                          const rofi::configuration::Module & newModule,
                          const rofi::configuration::Module & previousModule,
                          const Matrix & mPosition,
                          ModuleRenderInfo & moduleRenderInfo )
{
    if ( sameComponentTypes( newModule, previousModule ) ) {
        updateModulePositionInScene( newModule, mPosition, moduleRenderInfo );
        return;
    }
    removeModuleFromScene( renderer, moduleRenderInfo.componentActors );
    moduleRenderInfo.componentActors = addModuleToScene( renderer,
                                                         newModule,
                                                         mPosition,
                                                         moduleRenderInfo.activeConnectors );
}

void setActiveConnectors(
        const atoms::HandleSet< rofi::configuration::RoficomJoint > & roficomConnections,
        std::map< rofi::configuration::ModuleId, ModuleRenderInfo > & moduleRenderInfos )
{
    // TODO get from the inner state
    for ( auto & moduleRenderInfo : moduleRenderInfos ) {
        moduleRenderInfo.second.activeConnectors.clear();
    }
    for ( const auto & roficomConnection : roficomConnections ) {
        moduleRenderInfos[ roficomConnection.sourceModule ].activeConnectors.insert(
                roficomConnection.sourceConnector );
        moduleRenderInfos[ roficomConnection.destModule ].activeConnectors.insert(
                roficomConnection.destConnector );
    }
}


std::map< rofi::configuration::ModuleId, ModuleRenderInfo > addConfigurationToRenderer(
        vtkRenderer * renderer,
        const rofi::configuration::Rofibot & newConfiguration )
{
    assert( renderer != nullptr );

    std::map< rofi::configuration::ModuleId, ModuleRenderInfo > moduleRenderInfos;
    setActiveConnectors( newConfiguration.roficoms(), moduleRenderInfos );

    for ( const auto & moduleInfo : newConfiguration.modules() ) {
        assert( moduleInfo.module.get() );
        assert( moduleInfo.position && "The configuration has to be prepared" );
        moduleRenderInfos[ moduleInfo.module->getId() ].componentActors =
                addModuleToScene( renderer,
                                  *moduleInfo.module,
                                  *moduleInfo.position,
                                  moduleRenderInfos[ moduleInfo.module->getId() ]
                                          .activeConnectors );
    }

    assert( moduleRenderInfos.size() == newConfiguration.modules().size() );
    return moduleRenderInfos;
}
void updateConfigurationInRenderer(
        vtkRenderer * renderer,
        const rofi::configuration::Rofibot & newConfiguration,
        const rofi::configuration::Rofibot & previousConfiguration,
        std::map< rofi::configuration::ModuleId, ModuleRenderInfo > & moduleRenderInfos )
{
    assert( renderer != nullptr );

    setActiveConnectors( newConfiguration.roficoms(), moduleRenderInfos );

    auto previousModules =
            std::unordered_map< rofi::configuration::ModuleId,
                                std::reference_wrapper< const rofi::configuration::Module > >();
    for ( const auto & previousModuleInfo : previousConfiguration.modules() ) {
        assert( previousModuleInfo.module.get() );
        previousModules.emplace( previousModuleInfo.module->getId(), *previousModuleInfo.module );
    }

    for ( const auto & newModuleInfo : newConfiguration.modules() ) {
        assert( newModuleInfo.module.get() );
        assert( newModuleInfo.position && "The configuration has to be prepared" );
        auto & newModule = *newModuleInfo.module;
        auto & moduleRenderInfo = moduleRenderInfos[ newModule.getId() ];

        if ( auto previousModuleIt = previousModules.find( newModule.getId() );
             previousModuleIt != previousModules.end() )
        {
            updateModuleInScene( renderer,
                                 newModule,
                                 previousModuleIt->second,
                                 *newModuleInfo.position,
                                 moduleRenderInfo );
        }
        else {
            assert( moduleRenderInfo.componentActors.empty() );
            moduleRenderInfo.componentActors =
                    addModuleToScene( renderer,
                                      newModule,
                                      *newModuleInfo.position,
                                      moduleRenderInfo.activeConnectors );
        }
    }

    assert( moduleRenderInfos.size() == newConfiguration.modules().size() );
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
    assert( _renderer.Get() != nullptr );
    if ( auto newConfiguration = getCurrentConfig() ) {
        if ( _lastRenderedConfiguration != nullptr ) {
            updateConfigurationInRenderer( _renderer.Get(),
                                           *newConfiguration,
                                           *_lastRenderedConfiguration,
                                           _moduleRenderInfos );
        }
        else {
            assert( _moduleRenderInfos.empty() );
            _moduleRenderInfos = addConfigurationToRenderer( _renderer.Get(), *newConfiguration );
        }

        _lastRenderedConfiguration = std::move( newConfiguration );
        _renderWindow->Render();
    }
}
