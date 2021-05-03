#include "worldCreator.hpp"

#include <algorithm>
#include <cassert>
#include <sstream>

#include "IO.h"
#include "pidLoader.hpp"


constexpr std::array< const char *, 2 > shoeNames = { "shoeA", "shoeB" };

using namespace gazebo;


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

            ignition::math::Vector3d angles;
            lineStream >> config->pose.Pos() >> angles >> std::ws;
            config->pose.Rot() = ignition::math::Quaterniond( IGN_DTOR( angles ) );
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
            lineStream >> id >> alpha >> beta >> gamma >> std::ws;
            if ( !lineStream || !lineStream.eof() )
            {
                throw std::runtime_error( "Error while reading module" );
            }
            config->config.addModule( alpha, beta, gamma, id );
        }
        else if ( type == "E" )
        {
            auto edge = IO::readEdge( lineStream );
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
    using namespace ignition::math;

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
    using namespace ignition::math;

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

        auto modulePluginSdf = getElemByName< true >( moduleSdf, "plugin", "rofiModule" );
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
            auto modulePose = moveShoeToCenter( shoeId ) + matrixToPose( matrices.at( shoeId ) )
                              + config.pose;
            insertElement( moduleStateSdf, createRoficomState( roficomSdf, extended, modulePose ) );
        }
        assert( i == 6 );
    }

    setAttached( world, config.config );
}

sdf::SDFPtr loadFromFile( const std::string & filename )
{
    auto sdf = std::make_shared< sdf::SDF >();

    if ( !sdf::init( sdf ) )
    {
        throw std::runtime_error( "Unable to initialize sdf" );
    }

    auto fullPath = common::find_file( filename );
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
                        const ignition::math::Pose3d & beginPose )
{
    using namespace ignition::math;

    using TwoPoses = std::array< Pose3d, 2 >;
    static_assert( A == 0 && B == 1 );

    assert( moduleStateSdf );
    assert( moduleStateSdf->GetName() == "model" );

    TwoPoses matrixPoses = { matrixToPose( matrices[ A ] ), matrixToPose( matrices[ B ] ) };

    TwoPoses shoePoses = { moveShoeToCenter( A ) + matrixPoses[ A ],
                           moveShoeToCenter( B ) + matrixPoses[ B ] };

    TwoPoses bodyPoses = { moveShoeToCenter( A )
                                   + Pose3d( {}, { IGN_DTOR( module.getJoint( Alpha ) ), 0, 0 } )
                                   + matrixPoses[ A ],
                           moveShoeToCenter( B )
                                   + Pose3d( {}, { IGN_DTOR( module.getJoint( Beta ) ), 0, 0 } )
                                   + matrixPoses[ B ] };

    auto gammaRotation = Pose3d( {}, { 0, IGN_DTOR( module.getJoint( Gamma ) ), 0 } );
    if ( !equalPose( gammaRotation + bodyPoses[ A ], bodyPoses[ B ] ) )
    {
        throw std::runtime_error( "Error in computing the position of module bodies" );
    }

    TwoPoses origShoePoses = { Pose3d( -0.0265, -0.05, 0, 0, -0, 0 ),
                               Pose3d( -0.0265, 0.05, 0.0015, 0, -0, 0 ) };

    setPose( moduleStateSdf, bodyPoses[ A ] + beginPose );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "bodyA" ), bodyPoses[ A ] + beginPose );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "bodyB" ), bodyPoses[ B ] + beginPose );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "shoeA" ),
             origShoePoses[ A ] + shoePoses[ A ] + beginPose );
    setPose( getElemByNameOrCreate( moduleStateSdf, "link", "shoeB" ),
             origShoePoses[ B ] + shoePoses[ B ] + beginPose );
}

void setModulePIDPositionController( sdf::ElementPtr modulePluginSdf, const Module & module )
{
    PIDLoader::loadControllerValues( modulePluginSdf );

    for ( auto controllerSdf : getChildren( modulePluginSdf, "controller" ) )
    {
        auto jointName = getOnlyChild< true >( controllerSdf, "joint" )->Get< std::string >();
        Joint jointIndex = [ &jointName ]() {
            if ( jointName == "shoeARev" )
                return Joint::Alpha;
            if ( jointName == "shoeBRev" )
                return Joint::Beta;
            if ( jointName == "bodyRev" )
                return Joint::Gamma;
            throw std::runtime_error( "Unrecognized module joint in sdf" );
        }();

        auto positionControllerSdf = getOnlyChild< true >( controllerSdf, "position" );
        auto initTargetPosSdf = getOnlyChildOrCreate( positionControllerSdf, "init_target" );
        setValue( initTargetPosSdf, IGN_DTOR( module.getJoint( jointIndex ) ) );
    }
}

void setRoficomExtendedPlugin( sdf::ElementPtr pluginSdf, bool extended )
{
    assert( pluginSdf );

    setValue( getOnlyChildOrCreate( pluginSdf, "extend" ), extended );
}

sdf::ElementPtr createRoficomState( sdf::ElementPtr roficomSdf,
                                    bool extended,
                                    const ignition::math::Pose3d & parentPose )
{
    using namespace ignition::math;
    using namespace gazebo;

    assert( roficomSdf );
    assert( isRoFICoM( roficomSdf ) );

    auto roficomStateSdf = newElement( "model" );
    setAttribute( roficomStateSdf, "name", getAttribute< std::string >( roficomSdf, "name" ) );

    auto poseSdf = getOnlyChild< false >( roficomSdf, "pose" );
    auto roficomPose = ( poseSdf ? poseSdf->Get< Pose3d >() : Pose3d() ) + parentPose;
    setPose( roficomStateSdf, roficomPose );


    auto innerName = getRoFICoMInnerName( roficomSdf );

    auto innerSdf = getElemByName< true >( roficomSdf, "link", innerName );
    auto innerPoseSdf = getOnlyChild< false >( innerSdf, "pose" );
    Pose3d innerPose = ( innerPoseSdf ? innerPoseSdf->Get< Pose3d >() : Pose3d() ) + roficomPose;

    auto innerStateSdf = getElemByNameOrCreate( roficomStateSdf, "link", innerName );

    auto pos = Vector3d( 0, 0, extended ? 7e-3 : 0 ); // TODO read from sdf
    setPose( innerStateSdf, Pose3d( pos, {} ) + innerPose );

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

    setAttribute( modelSdf, "name", rofiName( rofiId ) );

    return modelSdf;
}

std::string getRoFICoMInnerName( sdf::ElementPtr roficomSdf )
{
    assert( roficomSdf );
    assert( isRoFICoM( roficomSdf ) );

    auto pluginSdf = getRoFICoMPluginSdf( roficomSdf );
    assert( pluginSdf );

    auto jointName = getOnlyChild< true >( pluginSdf, "joint" )->Get< std::string >();
    auto jointSdf = getElemByName< true >( roficomSdf, "joint", jointName );
    return getOnlyChild< true >( jointSdf, "child" )->Get< std::string >();
}

ignition::math::Pose3d matrixToPose( const Matrix & matrix )
{
    using namespace ignition;

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
