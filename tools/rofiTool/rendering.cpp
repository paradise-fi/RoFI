#include "rendering.hpp"

#include <atoms/resources.hpp>
#include <isoreconfig/geometry.hpp>
#include <isoreconfig/isomorphic.hpp>

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
    } );
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

        cache.emplace( type, t );
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
    auto axes = vtkSmartPointer< vtkAxesActor >::New();
    widget.SetOrientationMarker( axes );
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
        frameActor->SetScale( 1.0 / 95.0 );

        renderer.AddActor( frameActor );
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
    for ( auto & mInfo : world.modules() ) {
        assert( mInfo.absPosition && "The rofi world has to be prepared" );
        addModuleToScene( renderer,
                          *mInfo.module,
                          *mInfo.absPosition,
                          index,
                          activeConns[ mInfo.module->getId() ] );
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

void addPointToScene( vtkRenderer & renderer,
                      const Matrix & pointPosition,
                      std::array< double, 3 > colour,
                      double scale = 1.0 / 95.0 )
{
    auto posTrans = vtkSmartPointer< vtkTransform >::New();
    posTrans->SetMatrix( convertMatrix( pointPosition ) );

    auto filter = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    filter->SetTransform( posTrans );

    // Load blender object
    auto reader = vtkSmartPointer< vtkOBJReader >::New();
    ResourceFile modelFile = LOAD_RESOURCE_FILE( model_point_obj );
    reader->SetFileName( modelFile.name().c_str() );
    reader->Update();

    auto trans = vtkSmartPointer< vtkTransform >::New();
    trans->RotateX( 90 );
    auto t = vtkSmartPointer< vtkTransformPolyDataFilter >::New();
    t->SetInputConnection( reader->GetOutputPort() );
    t->SetTransform( trans );
    t->Update();

    filter->SetInputConnection( t->GetOutputPort() );

    auto frameMapper = vtkSmartPointer< vtkPolyDataMapper >::New();
    frameMapper->SetInputConnection( filter->GetOutputPort() );

    auto frameActor = vtkSmartPointer< vtkActor >::New();
    frameActor->SetMapper( frameMapper );
    frameActor->GetProperty()->SetColor( colour.data() );
    frameActor->GetProperty()->SetOpacity( 1.0 );
    frameActor->GetProperty()->SetFrontfaceCulling( true );
    frameActor->GetProperty()->SetBackfaceCulling( true );
    frameActor->SetPosition( pointPosition( 0, 3 ), pointPosition( 1, 3 ), pointPosition( 2, 3 ) );
    frameActor->SetScale( scale );

    renderer.AddActor( frameActor );
}

void buildRofiWorldPointsScene( vtkRenderer & renderer, RofiWorld world, bool showModules )
{
    using namespace rofi::isoreconfig;

    world.validate().get_or_throw_as< std::logic_error >();
    std::array< Positions, 2 > pts = decomposeRofiWorld( world );

    // merge [0] module points and [1] connection points
    for ( const Matrix & pos : pts[ 1 ] ) {
        pts[ 0 ].push_back( pos );
    }

    arma::mat coeff;    // principal component coefficients
    arma::mat score;    // projected data - points in new coordinate system
    arma::vec latent;   // eigenvalues of the covariance matrix of X - eigenvectors for PCA space
    arma::vec tsquared; // Hotteling's statistic for each sample
    princomp( coeff, score, latent, tsquared, cloudToScore( positionsToCloud( pts[ 0 ] ) ) );

    auto determinant = det( coeff );
    assert( std::abs( std::abs( determinant ) - 1 ) < ERROR_MARGIN );
    // If transformation is a reflection, reflect back along YZ plane
    // (otherwise there are rendering issues with modules)
    if ( determinant < 0 ) {
        score *= arma::mat{ { -1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
        coeff *= arma::mat{ { -1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    }

    Positions pos = cloudToPositions( scoreToCloud( score ) );
    assert( pos.size() == pts[ 0 ].size() );

    // Show module points (colour depends on index)
    assert( pts[ 0 ].size() >= pts[ 1 ].size() );
    for ( size_t i = 0; i < pts[ 0 ].size() - pts[ 1 ].size(); ++i ) {
        addPointToScene( renderer, pos[ i ], getModuleColor( int( i ) ) );
    }

    // Show connector points (green)
    for ( size_t i = pts[ 0 ].size() - pts[ 1 ].size(); i < pos.size(); ++i ) {
        addPointToScene( renderer, pos[ i ], { 0, 1, 0 }, 1 / 90.0 );
    }

    // Show centroid (black)
    addPointToScene( renderer, centroid( pos ), { 0, 0, 0 } );

    if ( !showModules )
        return;

    // Transpose and extend <coeff> to 4x4 matrix
    // (transposed <coeff> is PCA transformation)
    Matrix transf = arma::eye( 4, 4 );
    for ( int i = 0; i < 3; ++i ) {
        for ( int j = 0; j < 3; ++j ) {
            transf( i, j ) = coeff( j, i );
        }
    }

    // Translate reference points by the initial centroid in the direction of transf
    Matrix translation = matrices::translate( transf * ( -centroid( pts[ 0 ] ).col( 3 ) ) );

    // Transform rofi world reference points to new PCA coordinate system
    for ( auto j = world.referencePoints().begin(); j != world.referencePoints().end(); ++j ) {
        Vector newRefPoint = translation * j->refPoint;

        // Affix new reference point in new coordinate system <transf>
        const Component & comp = world.getModule( j->destModule )->components()[ j->destComponent ];
        world.disconnect( j.get_handle() );
        connect< RigidJoint >( comp, newRefPoint, transf );
    }

    auto result = world.prepare();
    assert( result );

    // get active (i.e. connected) connectors for each module within RofiWorld
    std::map< ModuleId, std::set< int > > activeConns;
    for ( const auto & roficom : world.roficomConnections() ) {
        auto sourceId = world.getModule( roficom.sourceModule )->getId();
        auto destId = world.getModule( roficom.destModule )->getId();
        activeConns[ sourceId ].insert( roficom.sourceConnector );
        activeConns[ destId ].insert( roficom.destConnector );
    }
    int index = 0;
    for ( auto & mInfo : world.modules() ) {
        assert( mInfo.absPosition && "The rofi world has to be prepared" );
        addModuleToScene( renderer,
                          *mInfo.module,
                          *mInfo.absPosition,
                          index,
                          activeConns[ mInfo.module->getId() ] );
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
