#include "worldCreator.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <sstream>

#include <gz/common/Util.hh>

#include <legacy/configuration/IO.h>
#include "pidLoader.hpp"


constexpr std::array< const char *, 2 > shoeNames = { "shoeA", "shoeB" };

using namespace gazebo;
using namespace rofi::configuration::matrices;

#ifndef WORLD_FILE
#define WORLD_FILE ""
#endif

#define STRINGIFY_IMPL( x ) #x
#define STRINGIFY( x ) STRINGIFY_IMPL( x )

namespace
{
std::string detectRofiRoot()
{
    if ( const char * rofiRoot = std::getenv( "ROFI_ROOT" ); rofiRoot && *rofiRoot )
    {
        return rofiRoot;
    }

    auto worldFile = std::filesystem::path( STRINGIFY( WORLD_FILE ) );
    if ( !worldFile.empty() )
    {
        return worldFile.parent_path().parent_path().parent_path().parent_path().string();
    }

    return {};
}

void configureGazeboFilePaths()
{
    static const bool configured = [] {
        const auto rofiRoot = detectRofiRoot();
        if ( rofiRoot.empty() )
        {
            return true;
        }

        const auto dataPath = std::filesystem::path( rofiRoot ) / "data" / "gazebo";
        const auto modelPath = dataPath / "models";
        const auto worldPath = dataPath / "worlds";

        if ( auto * systemPaths = gz::common::systemPaths() )
        {
            systemPaths->AddFilePaths( dataPath.string() + ":" + modelPath.string() + ":"
                                       + worldPath.string() );
        }

        gz::common::addFindFileURICallback(
                [ modelPath ]( const gz::common::URI & uri ) -> std::string {
                    if ( uri.Scheme() != "model" )
                    {
                        return {};
                    }

                    auto relativePath = uri.Path().Str();
                    while ( !relativePath.empty() && relativePath.front() == '/' )
                    {
                        relativePath.erase( relativePath.begin() );
                    }

                    auto candidate = modelPath / relativePath;
                    if ( std::filesystem::exists( candidate ) )
                    {
                        return candidate.string();
                    }

                    candidate /= "model.sdf";
                    if ( std::filesystem::exists( candidate ) )
                    {
                        return candidate.string();
                    }

                    return {};
                } );
        return true;
    }();
    (void)configured;
}
} // namespace


std::optional< ConfigWithPose > ConfigWithPose::tryReadConfiguration( std::istream & input )
{
    std::optional< ConfigWithPose > config;

    std::string line;
    std::streampos lineStart;

    while ( lineStart = input.tellg(), getline( input, line ) )
    {
        if ( line.empty() || line[ 0 ] == '#' )
        {
            continue; // Empty line or a comment
        }

        std::istringstream lineStream( line );
        std::string type;
        lineStream >> type;

        if ( type == "C" )
        {
            if ( config )
            {
                // Next configuration found, revert last line
                input.seekg( lineStart );
                return config;
            }

            config = ConfigWithPose();
            assert( config );

            lineStream >> std::ws;
            if ( lineStream.eof() )
            {
                continue;
            }

            gz::math::Vector3d angles;
            lineStream >> config->pose.Pos() >> angles;
            if ( !lineStream.eof() )
                lineStream >> std::ws;

            config->pose.Rot() = gz::math::Quaterniond( GZ_DTOR( angles ) );
            if ( !lineStream || !lineStream.eof() )
            {
                std::cerr << "Error while reading Configuration pose. Ignoring...\n";
                config->pose = {};
            }
            continue;
        }

        if ( !config )
        {
            throw std::runtime_error( "Configuration does not start with command 'C'" );
        }

        if ( type == "M" )
        {
            double alpha, beta, gamma;
            unsigned int id;
            lineStream >> id >> alpha >> beta >> gamma;
            if ( !lineStream.eof() )
                lineStream >> std::ws;

            if ( !lineStream || !lineStream.eof() )
            {
                throw std::runtime_error( "Error while reading module" );
            }
            config->config.addModule( alpha, beta, gamma, id );
        }
        else if ( type == "E" )
        {
            auto edge = IO::readEdge( lineStream );
            if ( !lineStream.eof() )
                lineStream >> std::ws;

            if ( !lineStream || !lineStream.eof() )
            {
                throw std::runtime_error( "Error while reading edge" );
            }
            config->config.addEdge( edge );
        }
        else
        {
            throw std::runtime_error( "Expected side module (M) or edge (E), got " + type + "." );
        }
    }

    return config;
}

sdf::SDFPtr createWorld( const std::string & worldPath,
                         const std::vector< ConfigWithPose > & configs )
{
    using namespace gz::math;

    configureGazeboFilePaths();
    sdf::setFindCallback( []( auto & filename ) { return gz::common::findFile( filename ); } );

    auto sdf = loadFromFile( worldPath );
    assert( sdf );
    assert( sdf->Root() );
    auto world = getOnlyChild< true >( sdf->Root(), "world" );

    for ( auto & config : configs )
    {
        addConfigurationToWorld( world, config );
    }
    return sdf;
}

void addConfigurationToWorld( sdf::ElementPtr world, const ConfigWithPose & config )
{
    using namespace gz::math;

    auto state = getOnlyChildOrCreate( world, "state" );
    setAttribute( state, "world_name", getAttribute< std::string >( world, "name" ) );

    auto distributorSdf = getDistributorPluginSdf( world );
    if ( !distributorSdf )
    {
        throw std::runtime_error( "Could not get distributor plugin" );
    }

    assert( config.config.getMatrices().size() == config.config.getModules().size() );
    for ( auto & [ id, module ] : config.config.getModules() )
    {
        assert( id == module.getId() );

        auto moduleSdf = newRoFIUniversalModule( id );
        insertElement( world, moduleSdf );


        auto moduleStateSdf = newElement( "model" );
        setAttribute( moduleStateSdf, "name", getAttribute< std::string >( moduleSdf, "name" ) );
        insertElement( state, moduleStateSdf );

        auto & matrices = config.config.getMatrices().at( id );
        setModulePosition( moduleStateSdf, module, matrices, config.pose );

        auto modulePluginSdf = getRoFIModulePluginSdf( moduleSdf );
        if ( !modulePluginSdf )
        {
            throw std::runtime_error( "Could not get module plugin" );
        }
        setModulePIDPositionController( modulePluginSdf, module );

        addModuleToDistributor( distributorSdf, id );

        const EdgeList & edges = config.config.getEdges().at( id );
        int i = 0;
        for ( auto roficomSdf : getChildren( moduleSdf, "model" ) )
        {
            if ( !isRoFICoM( roficomSdf ) )
            {
                continue;
            }
            assert( i < 6 );
            auto shoeId = i < 3 ? A : B;
            bool extended = edges.at( i ).has_value();
            i++;

            setRoficomExtendedPlugin( getRoFICoMPluginSdf( roficomSdf ), extended );

            auto shoeStateSdf =
                    getElemByName< true >( moduleStateSdf, "link", shoeNames.at( shoeId ) );
            auto modulePose = composePose(
                    composePose( moveShoeToCenter( shoeId ), matrixToPose( matrices.at( shoeId ) ) ),
                    config.pose );
            insertElement( moduleStateSdf, createRoficomState( roficomSdf, extended, modulePose ) );
        }
        assert( i == 6 );
    }

    setAttached( world, config.config );
}

sdf::SDFPtr loadFromFile( const std::string & filename )
{
    auto sdf = std::make_shared< sdf::SDF >();
    configureGazeboFilePaths();

    if ( !sdf::init( sdf ) )
    {
        throw std::runtime_error( "Unable to initialize sdf" );
    }

    auto fullPath = gz::common::findFile( filename );
    if ( fullPath.empty() )
    {
        std::cerr << "Could not find file '" + filename + "'. Did you set path variables?\n";
        throw std::runtime_error( "Could not find file " + filename );
    }

    sdf::Errors errors;
    if ( !sdf::readFile( fullPath, sdf, errors ) )
    {
        std::stringstream errorString;
        errorString << "Unable to load file[" << filename << "]\n";
        for ( const auto & e : errors )
        {
            errorString << e.Message() + "\n";
        }
        std::cerr << errorString.str();
        throw std::runtime_error( "Unable to load file" );
    }

    return sdf;
}

void addModuleToDistributor( sdf::ElementPtr distributorSdf, ID rofiId )
{
    auto rofiSdf = newElement( "rofi" );
    insertElement( distributorSdf, rofiSdf );

    insertElement( rofiSdf, newElemWithValue( "id", rofiId ) );
    insertElement( rofiSdf, newElemWithValue( "name", rofiName( rofiId ) ) );
}

void setModulePosition( sdf::ElementPtr moduleStateSdf,
                        const Module & module,
                        const std::array< Matrix, 2 > & matrices,
                        const gz::math::Pose3d & beginPose )
{
    using namespace gz::math;

    using TwoPoses = std::array< Pose3d, 2 >;
    static_assert( A == 0 && B == 1 );

    assert( moduleStateSdf );
    assert( moduleStateSdf->GetName() == "model" );

    TwoPoses matrixPoses = { matrixToPose( matrices[ A ] ), matrixToPose( matrices[ B ] ) };

    TwoPoses shoePoses = { composePose( moveShoeToCenter( A ), matrixPoses[ A ] ),
                           composePose( moveShoeToCenter( B ), matrixPoses[ B ] ) };

    TwoPoses bodyPoses = {
            composePose( composePose( moveShoeToCenter( A ),
                                      Pose3d( {}, { GZ_DTOR( module.getJoint( Alpha ) ), 0, 0 } ) ),
                         matrixPoses[ A ] ),
            composePose( composePose( moveShoeToCenter( B ),
                                      Pose3d( {}, { GZ_DTOR( module.getJoint( Beta ) ), 0, 0 } ) ),
                         matrixPoses[ B ] ) };

    auto gammaRotation = Pose3d( {}, { 0, GZ_DTOR( module.getJoint( Gamma ) ), 0 } );
    if ( !equalPose( composePose( gammaRotation, bodyPoses[ A ] ), bodyPoses[ B ] ) )
    {
        throw std::runtime_error( "Error in computing the position of module bodies" );
    }

    TwoPoses origShoePoses = { Pose3d( -0.0265, -0.05, 0, 0, -0, 0 ),
                               Pose3d( -0.0265, 0.05, 0.0015, 0, -0, 0 ) };

    setPose( moduleStateSdf, composePose( bodyPoses[ A ], beginPose ) );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "bodyA" ),
             composePose( bodyPoses[ A ], beginPose ) );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "bodyB" ),
             composePose( bodyPoses[ B ], beginPose ) );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "shoeA" ),
             composePose( composePose( origShoePoses[ A ], shoePoses[ A ] ), beginPose ) );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "shoeB" ),
             composePose( composePose( origShoePoses[ B ], shoePoses[ B ] ), beginPose ) );
}

void setModulePIDPositionController( sdf::ElementPtr modulePluginSdf, const Module & module )
{
    auto controllerValues = PIDLoader::loadControllerValues( modulePluginSdf );
    for ( auto & controllerValue : controllerValues )
    {
        Joint jointIndex = [ &controllerValue ]() {
            if ( controllerValue.jointName == "shoeAJoint" )
                return Joint::Alpha;
            if ( controllerValue.jointName == "shoeBJoint" )
                return Joint::Beta;
            if ( controllerValue.jointName == "bodyJoint" )
                return Joint::Gamma;
            throw std::runtime_error( "Unrecognized module joint in sdf" );
        }();

        controllerValue.position.initTarget = GZ_DTOR( module.getJoint( jointIndex ) );
    }

    for ( auto controllerSdf : getChildren( modulePluginSdf, "controller" ) )
    {
        controllerSdf->RemoveFromParent();
    }

    for ( const auto & controllerValue : controllerValues )
    {
        insertElement( modulePluginSdf, PIDLoader::createController( controllerValue ) );
    }
}

void setRoficomExtendedPlugin( sdf::ElementPtr pluginSdf, bool extended )
{
    assert( pluginSdf );

    setValue( getOnlyChildOrCreate( pluginSdf, "extend" ), extended );
}

sdf::ElementPtr createRoficomState( sdf::ElementPtr roficomSdf,
                                    bool extended,
                                    const gz::math::Pose3d & parentPose )
{
    using namespace gz::math;
    using namespace gazebo;

    assert( roficomSdf );
    assert( isRoFICoM( roficomSdf ) );

    auto roficomStateSdf = newElement( "model" );
    setAttribute( roficomStateSdf, "name", getAttribute< std::string >( roficomSdf, "name" ) );

    auto poseSdf = getOnlyChild< false >( roficomSdf, "pose" );
    auto roficomPose =
            composePose( poseSdf ? poseSdf->Get< Pose3d >() : Pose3d(), parentPose );
    setPose( roficomStateSdf, roficomPose );


    auto innerName = getRoFICoMInnerName( roficomSdf );

    auto roficomModelSdf = getOnlyChild< true >( roficomSdf, "model" );
    auto innerSdf = getElemByName< true >( roficomModelSdf, "link", innerName );
    auto innerPoseSdf = getOnlyChild< false >( innerSdf, "pose" );
    Pose3d innerPose =
            composePose( innerPoseSdf ? innerPoseSdf->Get< Pose3d >() : Pose3d(), roficomPose );

    auto innerStateSdf = getElemByNameOrCreate( roficomStateSdf, "link", innerName );

    auto pos = Vector3d( 0, 0, extended ? 7e-3 : 0 ); // TODO read from sdf
    setPose( innerStateSdf, composePose( Pose3d( pos, {} ), innerPose ) );

    return roficomStateSdf;
}

void setAttached( sdf::ElementPtr worldSdf, const Configuration & config )
{
    assert( worldSdf );
    assert( worldSdf->GetName() == "world" );

    auto attacherSdf = getAttacherPluginSdf( worldSdf );
    if ( !attacherSdf )
    {
        throw std::runtime_error( "Could not get attacher plugin" );
    }

    for ( const auto & [ id, edgeList ] : config.getEdges() )
    {
        for ( auto & edgeOpt : edgeList )
        {
            if ( !edgeOpt || id != std::min( edgeOpt->id1(), edgeOpt->id2() ) )
            {
                continue;
            }
            auto & edge = *edgeOpt;

            auto connectionSdf = newElement( "connection" );
            insertElement( attacherSdf, connectionSdf );

            insertElement( connectionSdf, newElemWithValue( "orientation", edge.ori() ) );

            auto moduleSdf1 = getElemByName< true >( worldSdf, "model", rofiName( edge.id1() ) );
            auto innerName1 = roficomInnerName( moduleSdf1, edge.side1(), edge.dock1() );
            insertElement( connectionSdf, newElemWithValue( "roficom", innerName1 ) );

            auto moduleSdf2 = getElemByName< true >( worldSdf, "model", rofiName( edge.id2() ) );
            auto innerName2 = roficomInnerName( moduleSdf2, edge.side2(), edge.dock2() );
            insertElement( connectionSdf, newElemWithValue( "roficom", innerName2 ) );
        }
    }
}


sdf::ElementPtr_V getModules( sdf::ElementPtr worldSdf )
{
    assert( worldSdf );

    auto children = getChildren( worldSdf, "model" );
    std::remove_if( children.begin(), children.end(), []( auto elem ) {
        return !isRoFIModule( elem );
    } );
    return children;
}

sdf::ElementPtr newRoFIUniversalModule( ID rofiId )
{
    auto sdf = loadFromFile( "model://universalModule/model.sdf" );
    assert( sdf );
    assert( sdf->Root() );

    auto modelSdf = getOnlyChild< true >( sdf->Root(), "model" );
    assert( isRoFIModule( modelSdf ) );

    for ( auto connectorSdf : getChildren( modelSdf, "model" ) )
    {
        auto includeSdf = getOnlyChild< false >( connectorSdf, "include" );
        if ( !includeSdf )
        {
            continue;
        }

        auto uriSdf = getOnlyChild< false >( includeSdf, "uri" );
        if ( !uriSdf || uriSdf->Get< std::string >() != "model://roficom" )
        {
            continue;
        }

        auto roficomSdf = loadFromFile( "model://roficom/model.sdf" );
        auto roficomModelSdf = getOnlyChild< true >( roficomSdf->Root(), "model" );
        insertElement( connectorSdf, roficomModelSdf );
        includeSdf->RemoveFromParent();
    }

    setAttribute( modelSdf, "name", rofiName( rofiId ) );

    return modelSdf;
}

std::string getRoFICoMInnerName( sdf::ElementPtr roficomSdf )
{
    assert( roficomSdf );
    assert( isRoFICoM( roficomSdf ) );

    auto roficomModelSdf = getOnlyChild< true >( roficomSdf, "model" );
    auto pluginSdf = getRoFICoMPluginSdf( roficomSdf );
    assert( pluginSdf );

    auto jointName = getOnlyChild< true >( pluginSdf, "joint" )->Get< std::string >();
    auto jointSdf = getElemByName< true >( roficomModelSdf, "joint", jointName );
    return getOnlyChild< true >( jointSdf, "child" )->Get< std::string >();
}

gz::math::Pose3d matrixToPose( const Matrix & matrix )
{
    using namespace gz;

    assert( matrix.n_rows == 4 );
    assert( matrix.n_cols == 4 );

    auto translateVec =
            math::Vector3d( matrix.col( 3 )[ 0 ], matrix.col( 3 )[ 1 ], matrix.col( 3 )[ 2 ] );

    auto rotMatrix = math::Matrix3d( matrix.row( 0 )[ 0 ],
                                     matrix.row( 0 )[ 1 ],
                                     matrix.row( 0 )[ 2 ],
                                     matrix.row( 1 )[ 0 ],
                                     matrix.row( 1 )[ 1 ],
                                     matrix.row( 1 )[ 2 ],
                                     matrix.row( 2 )[ 0 ],
                                     matrix.row( 2 )[ 1 ],
                                     matrix.row( 2 )[ 2 ] );

    return { translateVec / 10, math::Quaterniond( rotMatrix ) };
}
