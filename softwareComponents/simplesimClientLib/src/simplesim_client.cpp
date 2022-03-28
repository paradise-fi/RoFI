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
#include "ui_mainwindow.h"


using rofi::configuration::UniversalModule;
using rofi::simplesim::ChangeColor;
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

    assert( resourceMap.contains( type ) && "Unsupported component type specified" );

    if ( !cache.contains( type ) ) {
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
    for ( size_t i = 0; i < components.size(); i++ ) {
        auto frameActor = vtkSmartPointer< vtkActor >::New();
        frameActor->GetProperty()->SetColor( getModuleColor( newModule.getId() ).data() );
        frameActor->GetProperty()->SetOpacity( 1.0 );
        frameActor->GetProperty()->SetFrontfaceCulling( true );
        frameActor->GetProperty()->SetBackfaceCulling( true );
        frameActor->SetScale( 1. / 95. );

        assert( i <= INT_MAX );
        auto cPosition = mPosition
                       * newModule.getComponentRelativePosition( static_cast< int >( i ) );
        // make connected RoFICoMs connected visually
        if ( activeConnectors.contains( static_cast< int >( i ) ) ) {
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

    for ( size_t i = 0; i < components.size(); i++ ) {
        const auto & componentActor = moduleRenderInfo.componentActors[ i ];

        assert( i < INT_MAX );
        auto cPosition = mPosition
                       * newModule.getComponentRelativePosition( static_cast< int >( i ) );
        // make connected RoFICoMs connected visually
        if ( moduleRenderInfo.activeConnectors.contains( static_cast< int >( i ) ) ) {
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
        std::map< rofi::configuration::ModuleId, ModuleRenderInfo > & moduleRenderInfos,
        const rofi::configuration::Rofibot & newConfiguration )
{
    // TODO get from the inner state
    for ( auto & moduleRenderInfo : moduleRenderInfos ) {
        moduleRenderInfo.second.activeConnectors.clear();
    }
    for ( const auto & roficomConnection : roficomConnections ) {
        rofi::configuration::ModuleId source =
                newConfiguration.getModule( roficomConnection.sourceModule )->getId();
        rofi::configuration::ModuleId dest =
                newConfiguration.getModule( roficomConnection.destModule )->getId();

        moduleRenderInfos[ source ].activeConnectors.insert( roficomConnection.sourceConnector );
        moduleRenderInfos[ dest ].activeConnectors.insert( roficomConnection.destConnector );
    }
}


std::map< rofi::configuration::ModuleId, ModuleRenderInfo > addConfigurationToRenderer(
        vtkRenderer * renderer,
        const rofi::configuration::Rofibot & newConfiguration )
{
    assert( renderer != nullptr );

    std::map< rofi::configuration::ModuleId, ModuleRenderInfo > moduleRenderInfos;
    setActiveConnectors( newConfiguration.roficomConnections(),
                         moduleRenderInfos,
                         newConfiguration );

    for ( const auto & moduleInfo : newConfiguration.modules() ) {
        assert( moduleInfo.module.get() );
        assert( moduleInfo.absPosition && "The configuration has to be prepared" );
        moduleRenderInfos[ moduleInfo.module->getId() ].componentActors =
                addModuleToScene( renderer,
                                  *moduleInfo.module,
                                  *moduleInfo.absPosition,
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

    setActiveConnectors( newConfiguration.roficomConnections(),
                         moduleRenderInfos,
                         newConfiguration );

    auto previousModules =
            std::unordered_map< rofi::configuration::ModuleId,
                                std::reference_wrapper< const rofi::configuration::Module > >();
    for ( const auto & previousModuleInfo : previousConfiguration.modules() ) {
        assert( previousModuleInfo.module.get() );
        previousModules.emplace( previousModuleInfo.module->getId(), *previousModuleInfo.module );
    }

    for ( const auto & newModuleInfo : newConfiguration.modules() ) {
        assert( newModuleInfo.module.get() );
        assert( newModuleInfo.absPosition && "The configuration has to be prepared" );
        auto & newModule = *newModuleInfo.module;
        auto & moduleRenderInfo = moduleRenderInfos[ newModule.getId() ];

        if ( auto previousModuleIt = previousModules.find( newModule.getId() );
             previousModuleIt != previousModules.end() )
        {
            updateModuleInScene( renderer,
                                 newModule,
                                 previousModuleIt->second,
                                 *newModuleInfo.absPosition,
                                 moduleRenderInfo );
        } else {
            assert( moduleRenderInfo.componentActors.empty() );
            moduleRenderInfo.componentActors =
                    addModuleToScene( renderer,
                                      newModule,
                                      *newModuleInfo.absPosition,
                                      moduleRenderInfo.activeConnectors );
        }
    }

    assert( moduleRenderInfos.size() == newConfiguration.modules().size() );
}

SimplesimClient::SimplesimClient( OnSettingsCmdCallback onSettingsCmdCallback )
        : ui( std::make_unique< Ui::SimplesimClient >() )
        , _onSettingsCmdCallback( std::move( onSettingsCmdCallback ) )
{
    QMainWindow( nullptr );
    ui->setupUi( this );

    _renderer->SetBackground( 1.0, 1.0, 1.0 );
    _renderer->ResetCamera();
    _renderer->GetActiveCamera()->Zoom( 1.5 );
    _renderer->GetActiveCamera()->SetPosition( 0, 10, 5 );
    _renderer->GetActiveCamera()->SetViewUp( 0, 0, 1 );

    _renderWindow->SetSize( 1, 1 );
    _renderWindow->SetWindowName( "RoFI simulation" );
    _renderWindow->AddRenderer( _renderer.Get() );

    _renderWindowInteractor->SetRenderWindow( _renderWindow.Get() );
    _renderWindowInteractor->SetInteractorStyle( _interactorStyle.Get() );

    _renderWindowInteractor->Initialize();

    ui->widget->SetRenderWindow( _renderWindow.Get() );

    connect( ui->doubleSpinBox,
             SIGNAL( valueChanged( double ) ),
             this,
             SLOT( speedChanged( double ) ) );
    connect( ui->pauseButton, SIGNAL( clicked() ), this, SLOT( pauseButton() ) );
    connect( ui->treeWidget,
             SIGNAL( itemClicked( QTreeWidgetItem *, int ) ),
             this,
             SLOT( itemSelected( QTreeWidgetItem * ) ) );
    connect( ui->changeColor, SIGNAL( triggered() ), this, SLOT( changeColorWindow() ) );

    this->show();
}

SimplesimClient::~SimplesimClient()
{
    killTimer( _timerId );
}

void SimplesimClient::timerEvent( QTimerEvent * /* event */ )
{
    renderCurrentConfiguration();
}

void SimplesimClient::colorModule( rofi::configuration::ModuleId module,
                                   std::array< double, 3 > color,
                                   int component )
{
    assert( static_cast< int >( _moduleRenderInfos[ module ].componentActors.size() ) > component );

    if ( component == -1 ) {
        for ( auto & actor : _moduleRenderInfos[ module ].componentActors ) {
            actor->GetProperty()->SetColor( color.data() );
        }
    } else {
        _moduleRenderInfos[ module ].componentActors[ component ]->GetProperty()->SetColor(
                color.data() );
    }
}

void SimplesimClient::itemSelected( QTreeWidgetItem * selected )
{
    if ( _lastModule != -1 ) {
        colorModule( _lastModule, _lastColor );
    }

    int module;
    std::array< double, 3 > white = { { 1.0, 1.0, 1.0 } };

    if ( !selected->parent() ) {
        module = ui->treeWidget->indexOfTopLevelItem( selected );
        _moduleRenderInfos[ module ].componentActors.front()->GetProperty()->GetColor(
                _lastColor.data() );
        colorModule( module, white );
    } else if ( selected->parent() && !selected->parent()->parent() ) {
        module = ui->treeWidget->indexOfTopLevelItem( selected->parent() );
        _moduleRenderInfos[ module ].componentActors.front()->GetProperty()->GetColor(
                _lastColor.data() );
        colorModule( module, white );
    } else {
        module = ui->treeWidget->indexOfTopLevelItem( selected->parent()->parent() );
        int component = ui->treeWidget->topLevelItem( module )->child( 0 )->indexOfChild(
                selected );
        _moduleRenderInfos[ module ].componentActors[ component ]->GetProperty()->GetColor(
                _lastColor.data() );
        colorModule( module, white, component );
    }
    _lastModule = module;
}


void SimplesimClient::setColor( int color )
{
    const auto & toColor = _changeColorWindow->toColor;
    for ( int i = 0; i < static_cast< int >( toColor.size() ); ++i ) {
        if ( toColor[ i ] ) {
            if ( _lastModule == i ) {
                _lastColor = getModuleColor( color );
            } else {
                colorModule( i, getModuleColor( color ) );
            }
        }
    }
}

void SimplesimClient::changeColorWindow()
{
    if ( !_changeColorWindow ) {
        _changeColorWindow =
                std::make_unique< ChangeColor >( this, getCurrentConfig()->modules().size() );
        connect( _changeColorWindow.get(),
                 SIGNAL( pickedColor( int ) ),
                 this,
                 SLOT( setColor( int ) ) );
    }

    _changeColorWindow->show();
}

void SimplesimClient::pauseButton()
{
    bool paused = getCurrentSettings().paused();
    if ( paused ) {
        resume();
        std::cout << "Simulation playing\n";
    } else {
        pause();
        std::cout << "Simulation paused\n";
    }
}

void SimplesimClient::speedChanged( double speed )
{
    assert( speed < std::numeric_limits< float >::max() );
    changeSpeedRatio( static_cast< float >( speed ) );
    std::cout << "Speed changed to " << speed << '\n';
}

void SimplesimClient::clearRenderer()
{
    for ( auto & moduleRenderInfo : _moduleRenderInfos ) {
        removeModuleFromScene( _renderer.Get(), moduleRenderInfo.second.componentActors );
    }
    _moduleRenderInfos.clear();
    _lastRenderedConfiguration.reset();
}

void SimplesimClient::initInfoTree( const rofi::configuration::Rofibot & rofibot )
{
    int i = 0;
    for ( const auto & moduleInfo : rofibot.modules() ) {
        std::string str = "Module " + std::to_string( moduleInfo.module->getId() );
        QTreeWidgetItem * module = new QTreeWidgetItem( static_cast< QTreeWidget * >( nullptr ),
                                                        { QString( str.c_str() ) } );
        if ( ui->treeWidget->topLevelItemCount() <= i ) {
            ui->treeWidget->addTopLevelItem( module );
        }
        QTreeWidgetItem * components = new QTreeWidgetItem( module, { QString( "Components" ) } );
        for ( const auto & c : moduleInfo.module->components() ) {
            std::string comp = rofi::configuration::serialization::componentTypeToString( c.type );
            new QTreeWidgetItem( components, { QString( comp.c_str() ) } );
        }
        if ( auto * um = dynamic_cast< UniversalModule * >( moduleInfo.module.get() ) ) {
            for ( int j = 0; j < 6; ++j ) {
                std::string connector = um->translateComponent( j );
                ui->treeWidget->topLevelItem( i )
                        ->child( 0 )
                        ->child( j )
                        ->setText( 0, connector.c_str() );
            }
        }
        if ( moduleInfo.absPosition ) {
            std::string pos = "Position:\n" + IO::toString( *moduleInfo.absPosition );
            new QTreeWidgetItem( module, { QString( pos.c_str() ) } );
        }
        ++i;
    }
}

void SimplesimClient::updateInfoTree( const rofi::configuration::Rofibot & rofibot )
{
    int i = 0;
    for ( const auto & moduleInfo : rofibot.modules() ) {
        if ( moduleInfo.absPosition ) {
            std::string pos = "Position:\n" + IO::toString( *moduleInfo.absPosition );
            ui->treeWidget->topLevelItem( i )->child( 1 )->setText( 0, pos.c_str() );
        }
        ++i;
    }
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
        } else {
            assert( _moduleRenderInfos.empty() );
            _moduleRenderInfos = addConfigurationToRenderer( _renderer.Get(), *newConfiguration );
        }

        _lastRenderedConfiguration = std::move( newConfiguration );
        _renderWindow->Render();
    }

    updateInfoTree( *getCurrentConfig() );
}
