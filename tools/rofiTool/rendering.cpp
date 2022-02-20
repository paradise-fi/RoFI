#include "rendering.hpp"

#include <atoms/resources.hpp>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkOBJReader.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>

using namespace rofi::configuration;

vtkSmartPointer< vtkMatrix4x4 > convertMatrix( const Matrix& m ) {
    auto mat = vtkSmartPointer< vtkMatrix4x4 >::New();
    for ( int i = 0; i < 4; i++ )
        for ( int j = 0; j < 4; j++ ) {
            mat->SetElement( i, j, m(i , j) );
        }
    return mat;
}

constexpr std::array< double, 3 > getModuleColor( int moduleIdx ) {
    const std::array< std::array< uint8_t, 3 >, 7 > palette = {{
        { 191, 218, 112 },
        { 242, 202, 121 },
        { 218, 152, 207 },
        { 142, 202, 222 },
        { 104, 135, 205 },
        { 250, 176, 162 },
        { 234, 110, 111 } }};
    auto color = palette[ moduleIdx % palette.size() ];
    return { color[ 0 ] / 255.0, color[ 1 ] / 255.0, color[ 2 ] / 255.0 };
}

vtkAlgorithmOutput *getComponentModel( ComponentType type ) {
    using namespace rofi;

    static const std::map< ComponentType, std::function< ResourceFile() > >
        resourceMap({
            { ComponentType::UmShoe, LOAD_RESOURCE_FILE_LAZY( model_shoe_obj ) },
            { ComponentType::UmBody, LOAD_RESOURCE_FILE_LAZY( model_body_obj ) },
            { ComponentType::Roficom, LOAD_RESOURCE_FILE_LAZY( model_connector_obj ) }
        });
    static std::map< ComponentType, vtkSmartPointer< vtkTransformPolyDataFilter > > cache;

    assert( resourceMap.contains( type ) && "Unsupported component type specified" );

    if ( !cache.contains( type ) ) {
        auto reader = vtkSmartPointer<vtkOBJReader>::New();
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

void setupRenderer( vtkRenderer* renderer ) {
    renderer->SetBackground(1.0, 1.0, 1.0);
    renderer->ResetCamera();
}

void buildTemporarySceneShoeOnly( vtkRenderer* renderer, ComponentType component ) {
    vtkNew<vtkNamedColors> colors;
    vtkNew< vtkCylinderSource > cylinder;
    cylinder->SetResolution( 32 );
    vtkNew< vtkPolyDataMapper > bodyMapper;
    bodyMapper->SetInputConnection( getComponentModel( component ) );
    vtkNew<vtkActor> bodyActor;
    bodyActor->SetMapper(bodyMapper.Get());
    bodyActor->SetScale( 1 / 95.0 );
    bodyActor->GetProperty()->SetColor(
        colors->GetColor4d( "Tomato" ).GetData());
    renderer->AddActor( bodyActor.Get() );
}

void buildTemporaryScene( vtkRenderer* renderer ) {
    vtkNew<vtkNamedColors> colors;
    vtkNew< vtkCylinderSource > cylinder;
    cylinder->SetResolution( 32 );
    vtkNew< vtkPolyDataMapper > cylinderMapper;
    cylinderMapper->SetInputConnection( cylinder->GetOutputPort() );
    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper.Get());
    cylinderActor->GetProperty()->SetColor(
        colors->GetColor4d( "Tomato" ).GetData());
    cylinderActor->RotateX(30.0);
    cylinderActor->RotateY(-45.0);
    renderer->AddActor(cylinderActor.Get());
}

void addModuleToScene( vtkRenderer* renderer, Module& m,
                       const Matrix& mPosition, int moduleIndex, const std::set< int >& active_cons )
{
    auto moduleColor = getModuleColor( moduleIndex );
    const auto& components = m.components();
    for ( int i = 0; to_unsigned( i ) < components.size(); i++ ) {
        const auto& component = components[ to_unsigned( i ) ];
        auto cPosition = m.getComponentPosition( i, mPosition );
        // make connected RoFICoMs connected visually
        if ( active_cons.contains( i ) )
            cPosition = cPosition * rofi::configuration::matrices::translate( { -0.05, 0, 0 } );

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
    }
}

void buildConfigurationScene( vtkRenderer* renderer, Rofibot& bot ) {
    // get active (i.e. connected) connectors for each module within Rofibot
    std::map< ModuleId, std::set< int > > active_cons;
    for ( const auto& roficom : bot.roficoms() ) {
        active_cons[ bot.getModule( roficom.sourceModule )->getId() ].insert( roficom.sourceConnector );
        active_cons[ bot.getModule( roficom.destModule )->getId()   ].insert( roficom.destConnector );
    }

    int index = 0;
    for ( auto& mInfo : bot.modules() ) {
        assert( mInfo.position && "The configuration has to be prepared" );
        if ( !active_cons.contains( mInfo.module->getId() ) )
            active_cons[ mInfo.module->getId() ] = {};
        addModuleToScene( renderer, *mInfo.module, *mInfo.position, index, active_cons[ mInfo.module->getId() ] );
        index++;
    }
}

void renderConfiguration( Rofibot configuration, const std::string& configName ) {
        vtkNew< vtkRenderer > renderer;
    setupRenderer( renderer.Get() );
    buildConfigurationScene( renderer.Get(), configuration );

    vtkNew< vtkRenderWindow > renderWindow;
    renderWindow->AddRenderer( renderer.Get() );
    renderWindow->SetWindowName( ( "Preview of " + configName ).c_str() );

    // Setup main window loop
    vtkNew< vtkRenderWindowInteractor > renderWindowInteractor;
    renderWindowInteractor->SetRenderWindow( renderWindow.Get() );

    vtkNew< vtkAxesActor > axes;
    vtkNew< vtkOrientationMarkerWidget > widget;
    widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    widget->SetOrientationMarker( axes.Get() );
    widget->SetInteractor( renderWindowInteractor.Get() );
    widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
    widget->SetEnabled( 1 );
    widget->InteractiveOn();

    // Start main window loop
    renderWindow->Render();
    renderWindowInteractor->Start();
}
