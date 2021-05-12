#include "preview.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <functional>
#include <map>

#include <rofibot.h>
#include <universalModule.h>

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

vtkSmartPointer< vtkMatrix4x4 > convertMatrix( const Matrix& m ) {
    auto mat = vtkSmartPointer< vtkMatrix4x4 >::New();
    for ( int i = 0; i < 4; i++ )
        for ( int j = 0; j < 4; j++ ) {
            mat->SetElement( i, j, m(i , j) );
        }
    return mat;
}

std::array< double, 3 > getModuleColor( int moduleIdx ) {
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

vtkAlgorithmOutput *getComponentModel( rofi::ComponentType type ) {
    using namespace rofi;

    static const std::map< ComponentType, std::function< ResourceFile() > >
        resourceMap({
            { ComponentType::UmShoe, LOAD_RESOURCE_FILE_LAZY( model_body_obj ) },
            { ComponentType::UmBody, LOAD_RESOURCE_FILE_LAZY( model_shoe_obj ) },
            { ComponentType::Roficom, LOAD_RESOURCE_FILE_LAZY( model_connector_obj ) }
        });
    static std::map< ComponentType, vtkSmartPointer< vtkOBJReader > > cache;

    assert( resourceMap.count( type ) == 1 && "Unsupported component type specified" );

    if ( cache.count( type ) == 0 ) {
        auto reader = vtkSmartPointer<vtkOBJReader>::New();
        ResourceFile modelFile = resourceMap.find( type )->second();
        reader->SetFileName( modelFile.name().c_str() );
        reader->Update();
        cache.insert( { type, reader } );
    }
    return cache[ type ]->GetOutputPort();
}

void setupRenderer( vtkRenderer* renderer ) {
    renderer->SetBackground(1.0, 1.0, 1.0);
    renderer->ResetCamera();
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
        colors->GetColor4d("Tomato").GetData());
    cylinderActor->RotateX(30.0);
    cylinderActor->RotateY(-45.0);
    renderer->AddActor(cylinderActor.Get());
}

void addModuleToScene( vtkRenderer* renderer, rofi::Module& m,
                       const Matrix& mPosition, int moduleIndex )
{
    auto moduleColor = getModuleColor( moduleIndex );
    const auto& components = m.components();
    for ( int i = 0; i != components.size(); i++ ) {
        const auto& component = components[ i ];
        auto cPosition = m.getComponentPosition( i, mPosition );

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
        frameActor->SetPosition( cPosition(0, 3), cPosition(1, 3), cPosition(2, 3) );
        frameActor->SetScale( 1 / 95.0 );

        renderer->AddActor( frameActor );
    }
}

void buildConfigurationScene( vtkRenderer* renderer, rofi::Rofibot& bot ) {
    int index = 0;
    for ( auto& mInfo : bot.modules() ) {
        assert( mInfo.position && "The configuration has to be prepared" );
        addModuleToScene( renderer, *mInfo.module, *mInfo.position, index );
        index++;
    }
}

static auto& command = Dim::Cli().command( "preview" )
    .desc( "Interactively preview a configuration" );
static auto& inputFile = command.opt< std::string >( "<FILE>" )
    .desc("Specify source file");

int preview( Dim::Cli & cli ) {
    auto cfgFile = std::ifstream( *inputFile );
    if ( !cfgFile.is_open() )
        throw std::runtime_error( "Cannot open file '" + *inputFile + "'" );

    auto configuration = rofi::readOldConfigurationFormat( cfgFile );
    rofi::connect< rofi::RigidJoint >(
        configuration.getModule( 0 )->bodies()[ 0 ],
        Vector( {0, 0, 0, 1} ),
        identity );
    configuration.prepare();

    vtkNew< vtkRenderer > renderer;
    setupRenderer( renderer.Get() );
    buildConfigurationScene( renderer.Get(), configuration );

    vtkNew< vtkRenderWindow > renderWindow;
    renderWindow->AddRenderer( renderer.Get() );
    renderWindow->SetWindowName( ( "Preview of " + *inputFile ).c_str() );

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

    return 0;
}