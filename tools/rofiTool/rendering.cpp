#include "rendering.hpp"

#include <string_view>

#include <atoms/resources.hpp>
#include <shapeReconfig/isomorphic.hpp>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
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


using namespace rofi::configuration;

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

constexpr std::array< double, 3 > getModuleColor( int moduleIdx )
{
    constexpr auto palette = std::array< std::array< uint8_t, 3 >, 7 >{ {
            { 191, 218, 112 },
            { 242, 202, 121 },
            { 218, 152, 207 },
            { 142, 202, 222 },
            { 104, 135, 205 },
            { 250, 176, 162 },
            { 234, 110, 111 },
    } };
    auto color = palette[ moduleIdx % palette.size() ];
    return std::array{ color[ 0 ] / 255.0, color[ 1 ] / 255.0, color[ 2 ] / 255.0 };
}

vtkAlgorithmOutput * getComponentModel( ComponentType type )
{
    static const auto resourceMap = std::map< ComponentType, std::function< ResourceFile() > >( {
            { ComponentType::Roficom, LOAD_RESOURCE_FILE_LAZY( model_connector_obj ) },
            { ComponentType::UmBody, LOAD_RESOURCE_FILE_LAZY( model_body_obj ) },
            { ComponentType::UmShoe, LOAD_RESOURCE_FILE_LAZY( model_shoe_obj ) },
            { ComponentType::CubeBody, LOAD_RESOURCE_FILE_LAZY( model_cube_obj ) },
            { ComponentType::Cylinder, LOAD_RESOURCE_FILE_LAZY( model_cylinder_obj ) },
    } );
    static std::map< ComponentType, vtkSmartPointer< vtkTransformPolyDataFilter > > cache;

    assert( resourceMap.contains( type ) && "Unsupported component type specified" );

    if ( !cache.contains( type ) ) {
        vtkNew< vtkOBJReader > reader;
        ResourceFile modelFile = resourceMap.find( type )->second();
        reader->SetFileName( modelFile.name().c_str() );
        reader->Update();

        vtkNew< vtkTransform > trans;
        trans->RotateX( 90 );
        auto t = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
        t->SetInputConnection( reader->GetOutputPort() );
        t->SetTransform( trans.Get() );
        t->Update();

        cache.emplace( type, std::move( t ) );
    }
    return cache.at( type )->GetOutputPort();
}

void setupRenderer( vtkRenderer & renderer )
{
    renderer.SetBackground( 1.0, 1.0, 1.0 );
    renderer.ResetCamera();

    auto activeCamera = renderer.GetActiveCamera();
    assert( activeCamera );
    activeCamera->Zoom( 1 );
    activeCamera->SetPosition( 0, 10, 5 );
    activeCamera->SetViewUp( 0, 0, 1 );
}

void setupRenderWindow( vtkRenderWindow * renderWindow,
                        vtkRenderWindowInteractor * renderWindowInteractor,
                        const std::string & displayName )
{
    assert( renderWindow );
    assert( renderWindowInteractor );

    if ( auto screenSize = renderWindow->GetScreenSize() ) {
        renderWindow->SetPosition( screenSize[ 0 ] / 4, screenSize[ 1 ] / 6 );
        renderWindow->SetSize( screenSize[ 0 ] / 2, screenSize[ 1 ] / 2 );
    }
    renderWindow->SetWindowName( displayName.c_str() );

    renderWindowInteractor->SetRenderWindow( renderWindow );
    vtkNew< vtkInteractorStyleTrackballCamera > interactorStyle;
    renderWindowInteractor->SetInteractorStyle( interactorStyle.Get() );
    renderWindowInteractor->Initialize();
}

void addAxesWidget( vtkOrientationMarkerWidget & widget,
                    vtkRenderWindowInteractor * renderWindowInteractor )
{
    assert( renderWindowInteractor );

    widget.SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    vtkNew< vtkAxesActor > axes;
    widget.SetOrientationMarker( axes.Get() );
    widget.SetInteractor( renderWindowInteractor );
    widget.SetViewport( 0.0, 0.0, 0.4, 0.4 );
    widget.SetEnabled( 1 );
    widget.InteractiveOn();
}

void addModuleToScene( vtkRenderer & renderer,
                       const Module & m,
                       const Matrix & mPosition,
                       int moduleIndex,
                       const std::set< int > & activeConns )
{
    auto moduleColor = getModuleColor( moduleIndex );
    const auto & components = m.components();
    for ( int i = 0; to_unsigned( i ) < components.size(); i++ ) {
        const auto & component = components[ to_unsigned( i ) ];
        Matrix cPosition = mPosition * m.getComponentRelativePosition( i );
        // make connected RoFICoMs connected visually
        if ( activeConns.contains( i ) ) {
            cPosition = cPosition * matrices::translate( { -0.05, 0, 0 } );
        }

        vtkNew< vtkTransform > posTrans;
        posTrans->SetMatrix( convertMatrix( cPosition ) );

        vtkNew< vtkTransformPolyDataFilter > filter;
        filter->SetTransform( posTrans.Get() );
        filter->SetInputConnection( getComponentModel( component.type ) );

        vtkNew< vtkPolyDataMapper > frameMapper;
        frameMapper->SetInputConnection( filter->GetOutputPort() );

        vtkNew< vtkActor > frameActor;
        frameActor->SetMapper( frameMapper.Get() );
        frameActor->GetProperty()->SetColor( moduleColor.data() );
        frameActor->GetProperty()->SetOpacity( 1.0 );
        frameActor->GetProperty()->SetFrontfaceCulling( true );
        frameActor->GetProperty()->SetBackfaceCulling( true );
        frameActor->SetPosition( cPosition( 0, 3 ), cPosition( 1, 3 ), cPosition( 2, 3 ) );
        frameActor->SetScale( 1.0 / 95.0 );

        renderer.AddActor( frameActor.Get() );
    }
}

void buildRofiWorldScene( vtkRenderer & renderer, const RofiWorld & world )
{
    assert( world.isPrepared() && "The rofi world has to be prepared" );

    // get active (i.e. connected) connectors for each module within RofiWorld
    std::map< ModuleId, std::set< int > > activeConns;
    for ( const auto & roficom : world.roficomConnections() ) {
        auto sourceId = world.getModule( roficom.sourceModule )->getId();
        auto destId = world.getModule( roficom.destModule )->getId();
        activeConns[ sourceId ].insert( roficom.sourceConnector );
        activeConns[ destId ].insert( roficom.destConnector );
    }

    int index = 0;
    for ( const auto & [ rModule, absPosition ] : world.modulesWithAbsPos() ) {
        addModuleToScene( renderer, rModule, absPosition, index, activeConns[ rModule.getId() ] );
        index++;
    }
}

void renderRofiWorld( const RofiWorld & world, const std::string & displayName )
{
    assert( world.isPrepared() && "The rofi world has to be prepared" );
    assert( world.isValid() && "The rofi world has to be valid" );

    vtkNew< vtkRenderer > renderer;
    vtkNew< vtkRenderWindow > renderWindow;
    vtkNew< vtkRenderWindowInteractor > renderWindowInteractor;
    setupRenderer( *renderer.Get() );
    setupRenderWindow( renderWindow.Get(), renderWindowInteractor.Get(), displayName );
    renderWindow->AddRenderer( renderer.Get() );

    vtkNew< vtkOrientationMarkerWidget > widget;
    addAxesWidget( *widget.Get(), renderWindowInteractor.Get() );

    buildRofiWorldScene( *renderer.Get(), world );

    // Start main window loop
    renderWindow->Render();
    renderWindowInteractor->Start();
}

class [[nodiscard]] SequenceRenderer {
private:
    void removeCurrentRenderer()
    {
        assert( renderWindow.HasRenderer( renderers[ currentRenderer ].Get() ) );
        renderWindow.RemoveRenderer( renderers[ currentRenderer ].Get() );
    }
    void setRenderer( int newRenderer )
    {
        assert( !renderers.empty() );
        assert( newRenderer >= 0 );
        assert( to_unsigned( newRenderer ) < renderers.size() );

        currentRenderer = to_unsigned( newRenderer );
        renderWindow.AddRenderer( renderers[ currentRenderer ].Get() );
        renderWindow.SetWindowName( getCurrentDisplayName().c_str() );
    }

    static void keypressEventCallback( vtkObject * caller,
                                       [[maybe_unused]] unsigned long eventId,
                                       void * clientData,
                                       void * /* callData */ )
    {
        using namespace std::string_view_literals;

        assert( eventId == vtkCommand::KeyPressEvent );
        assert( dynamic_cast< vtkRenderWindowInteractor * >( caller ) );
        assert( clientData );

        auto & self = *reinterpret_cast< SequenceRenderer * >( clientData );

        auto & renderWindowInteractor = *static_cast< vtkRenderWindowInteractor * >( caller );
        auto keyPressed = std::string_view( renderWindowInteractor.GetKeySym() );
        if ( keyPressed == "Left"sv ) {
            if ( !self.setPrevRenderer() ) {
                std::cout << "Already at the first world in sequence\n";
            }
            self.renderWindow.Render();
        } else if ( keyPressed == "Right"sv ) {
            if ( !self.setNextRenderer() ) {
                std::cout << "Already at the last world in sequence\n";
            }
            self.renderWindow.Render();
        }
    }

public:
    SequenceRenderer( std::span< const RofiWorld > worlds,
                      std::string displayName,
                      vtkRenderWindow & renderWindow )
            : renderers( worlds.size() )
            , displayName( std::move( displayName ) )
            , renderWindow( renderWindow )
    {
        assert( !worlds.empty() );
        assert( worlds.size() == renderers.size() );

        for ( size_t i = 0; i < worlds.size(); i++ ) {
            assert( worlds[ i ].isValid() && "All rofi worlds have to be valid" );
            setupRenderer( *renderers[ i ].Get() );
            buildRofiWorldScene( *renderers[ i ].Get(), worlds[ i ] );
        }

        // Use the same camera for all renderers
        for ( size_t i = 1; i < worlds.size(); i++ ) {
            renderers[ i ]->SetActiveCamera( renderers[ 0 ]->GetActiveCamera() );
        }

        setRenderer( 0 );
    }

    bool setNextRenderer()
    {
        if ( currentRenderer + 1 >= renderers.size() ) {
            return false;
        }
        updateRenderer( static_cast< int >( currentRenderer ) + 1 );
        return true;
    }
    bool setPrevRenderer()
    {
        if ( currentRenderer <= 0 ) {
            return false;
        }
        updateRenderer( static_cast< int >( currentRenderer ) - 1 );
        return true;
    }

    void updateRenderer( int newRenderer )
    {
        if ( newRenderer < 0 || to_unsigned( newRenderer ) >= renderers.size() ) {
            std::cerr << "Renderer " << newRenderer << "is out of bounds\n";
            return;
        }

        removeCurrentRenderer();
        setRenderer( newRenderer );
    }

    std::string getCurrentDisplayName() const
    {
        return std::string( "(" ) + std::to_string( currentRenderer + 1 ) + "/"
             + std::to_string( renderers.size() ) + ") " + displayName;
    }

    void setCallback( vtkRenderWindowInteractor & renderWindowInteractor )
    {
        vtkNew< vtkCallbackCommand > keypressCallback;
        keypressCallback->SetCallback( SequenceRenderer::keypressEventCallback );
        keypressCallback->SetClientData( this );
        renderWindowInteractor.AddObserver( vtkCommand::KeyPressEvent, keypressCallback.Get() );
    }

private:
    size_t currentRenderer = 0;
    std::vector< vtkNew< vtkRenderer > > renderers;
    std::string displayName;
    vtkRenderWindow & renderWindow;
};

void renderRofiWorldSequence( std::span< const RofiWorld > worlds, const std::string & displayName )
{
#ifndef NDEBUG
    for ( const auto & world : worlds ) {
        assert( world.isPrepared() && "All rofi worlds have to be prepared" );
        assert( world.isValid() && "All rofi worlds have to be valid" );
    }
#endif

    vtkNew< vtkRenderWindow > renderWindow;
    vtkNew< vtkRenderWindowInteractor > renderWindowInteractor;
    setupRenderWindow( renderWindow.Get(), renderWindowInteractor.Get(), displayName );

    auto sequenceRenderer = SequenceRenderer( worlds, displayName, *renderWindow.Get() );
    sequenceRenderer.setCallback( *renderWindowInteractor.Get() );

    vtkNew< vtkOrientationMarkerWidget > widget;
    addAxesWidget( *widget.Get(), renderWindowInteractor.Get() );

    // Start main window loop
    renderWindow->Render();
    renderWindowInteractor->Start();
}

void addPointToScene( vtkRenderer & renderer,
                      const Matrix & pointPosition,
                      std::array< double, 3 > colour,
                      double scale = 1.0 / 95.0 )
{
    vtkNew< vtkTransform > posTrans;
    posTrans->SetMatrix( convertMatrix( pointPosition ) );

    vtkNew< vtkTransformPolyDataFilter > filter;
    filter->SetTransform( posTrans.Get() );

    // Load blender object
    vtkNew< vtkOBJReader > reader;
    ResourceFile modelFile = LOAD_RESOURCE_FILE( model_point_obj );
    reader->SetFileName( modelFile.name().c_str() );
    reader->Update();

    vtkNew< vtkTransform > trans;
    trans->RotateX( 90 );
    vtkNew< vtkTransformPolyDataFilter > t;
    t->SetInputConnection( reader->GetOutputPort() );
    t->SetTransform( trans.Get() );
    t->Update();

    filter->SetInputConnection( t->GetOutputPort() );

    vtkNew< vtkPolyDataMapper > frameMapper;
    frameMapper->SetInputConnection( filter->GetOutputPort() );

    vtkNew< vtkActor > frameActor;
    frameActor->SetMapper( frameMapper.Get() );
    frameActor->GetProperty()->SetColor( colour.data() );
    frameActor->GetProperty()->SetOpacity( 1.0 );
    frameActor->GetProperty()->SetFrontfaceCulling( true );
    frameActor->GetProperty()->SetBackfaceCulling( true );
    frameActor->SetPosition( pointPosition( 0, 3 ), pointPosition( 1, 3 ), pointPosition( 2, 3 ) );
    frameActor->SetScale( scale );

    renderer.AddActor( frameActor.Get() );
}

void buildRofiWorldPointsScene( vtkRenderer & renderer, RofiWorld world, bool showModules )
{
    using namespace rofi::shapereconfig;

    world.validate().get_or_throw_as< std::logic_error >();
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( world );

    // merge module points and connection points
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );

    // Transform all points to PCA coordinate system
    Cloud cop( modulePoints );
    std::vector< Vector > newPts = cop.toVectors();

    // Show points (colour depends on index)
    for ( size_t pt = 0; pt < newPts.size(); ++pt )
        addPointToScene( renderer, pointMatrix( newPts[pt] ), getModuleColor( int(pt) ) );

    // Show centroid (black) - PCA shifts centroid to (0, 0, 0)
    addPointToScene( renderer, arma::eye( 4, 4 ), { 0, 0, 0 } );

    if ( !showModules ) return;
    
    // Get and extend transformation to 4x4 matrix
    arma::mat transf = cop.transformation();
    transf.resize( 4, 4 );
    transf.row(3).zeros();
    transf.col(3).zeros();
    transf(3, 3) = 1;

    Vector oldCentroid = centroid( modulePoints );
    // Transform configuration reference points to new PCA coordinate system
    for ( auto j = world.referencePoints().begin(); j != world.referencePoints().end(); ++j )
    {
        Vector newRefPoint = transf * ( j->refPoint - oldCentroid );
        newRefPoint(3) = 0;
        
        // Affix new reference point in new coordinate system <transf>
        const Component & comp = world.getModule( j->destModule )->components()[ j->destComponent ];
        world.disconnect( j.get_handle() );
        connect< RigidJoint >( comp, newRefPoint, transf );
    }

    world.prepare().get_or_throw_as< std::runtime_error >();

    // get active (i.e. connected) connectors for each module within RofiWorld
    std::map< ModuleId, std::set< int > > activeConns;
    for ( const auto & roficom : world.roficomConnections() ) {
        auto sourceId = world.getModule( roficom.sourceModule )->getId();
        auto destId = world.getModule( roficom.destModule )->getId();
        activeConns[ sourceId ].insert( roficom.sourceConnector );
        activeConns[ destId ].insert( roficom.destConnector );
    }
    int index = 0;
    for ( const auto & [ rModule, absPosition ] : world.modulesWithAbsPos() ) {
        addModuleToScene( renderer, rModule, absPosition, index, activeConns[ rModule.getId() ] );
        index++;
    }
}

void renderPoints( RofiWorld world, const std::string & displayName, bool showModules )
{
    vtkNew< vtkRenderer > renderer;
    vtkNew< vtkRenderWindow > renderWindow;
    vtkNew< vtkRenderWindowInteractor > renderWindowInteractor;
    setupRenderer( *renderer.Get() );
    setupRenderWindow( renderWindow.Get(), renderWindowInteractor.Get(), displayName );
    renderWindow->AddRenderer( renderer.Get() );

    vtkNew< vtkOrientationMarkerWidget > widget;
    addAxesWidget( *widget.Get(), renderWindowInteractor.Get() );

    buildRofiWorldPointsScene( *renderer.Get(), std::move( world ), showModules );

    // Start main window loop
    renderWindow->Render();
    renderWindowInteractor->Start();
}
