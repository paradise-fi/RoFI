#pragma once

#include <configuration/rofiworld.hpp>

namespace rofi::configuration {

using namespace rofi::configuration::matrices;

class Support : public Module {

    static int roficomCount( int width, int height )
    {
        return ( width == 1 || height == 1 ) ? 4 * std::max( width, height ) + 2 : 2 * width + 4;
    }

    static int cubeCount( int width, int height )
    {
        return std::max( width, height );
    }

    static std::vector< Component > _initComponents( int width, int height )
    {
        assert( (width == 1 || height == 1 || width == height) && "Support must have isometric dimensions or height/width of one" );
        auto components = std::vector< Component >( roficomCount( width, height ), Component { ComponentType::Roficom, {}, {}, nullptr } );
        for ( int i = 1; i < cubeCount( width, height ); ++i ) 
        {
            components.push_back( Component{ ComponentType::CubeBody, {}, {}, nullptr } ); 
            components.push_back( Component{ ComponentType::Cylinder, {}, {}, nullptr } );
        }
        components.push_back( Component{ ComponentType::CubeBody, {}, {}, nullptr } ); 
        return components;
    }

    static std::vector< ComponentJoint > _initJoints( int length )
    {
        assert( length > 1 && "Length of a straight support must be at least 2" );

        std::vector< ComponentJoint > joints;
        int roficoms = roficomCount( 1, length );
        int compCount = roficoms + 2 * cubeCount( 1, length ) - 1;

        joints.push_back( makeComponentJoint< RigidJoint >( roficoms, 0, rotate( Angle::pi / 2, { 0, 0, 1 } ) ) );
        int currRoficom = 1;
        for ( int currComp = roficoms; currComp < compCount - 1; currComp += 2 )
        {
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currComp + 1, // Cube to cylinder
                rotate( Angle::pi / 2, { 0, 0, -1 } ) * translate( { -0.5, 0, 0 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp + 1, currComp + 2, // Cylinder to cube
                translate( { -0.5, 0, 0 } ) * rotate( Angle::pi / 2, { 0, 0, 1 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom, identity ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom + 1, rotate( Angle::pi / 2, { 0, 1, 0 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom + 2, rotate( Angle::pi / 2, { 0, -1, 0 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom + 3, rotate( Angle::pi, { 0, 1, 0 } ) ) );
            currRoficom += 4;
        }
        assert( currRoficom == roficoms - 5 );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom, identity ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 1, rotate( Angle::pi / 2, { 0, 1, 0 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 2, rotate( Angle::pi / 2, { 0, -1, 0 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 3, rotate( Angle::pi, { 0, 1, 0 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 4, rotate( Angle::pi / 2, { 0, 0, -1 } ) ) );
            
        // std::vector< ComponentJoint > joints = {
        //     makeComponentJoint< RigidJoint >( CubeA, 0, identity ),
        //     makeComponentJoint< RigidJoint >( CubeA, 1, rotate( Angle::pi / 2, { 0, -1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeA, 2, rotate( Angle::pi / 2, { 0, 1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeA, 3, rotate( Angle::pi / 2, { 0, 0, 1 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeA, 4, rotate( Angle::pi / 2, { 0, 0, -1 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 5, identity ),
        //     makeComponentJoint< RigidJoint >( CubeB, 6, rotate( Angle::pi / 2, { 0, -1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 7, rotate( Angle::pi / 2, { 0, 1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 8, rotate( Angle::pi / 2, { 0, 0, 1 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 9, rotate( Angle::pi / 2, { 0, 0, -1 } ) ),

        //     makeComponentJoint< RigidJoint >( RigidBody, CubeA, identity ), // Center is in CubeA
        //     makeComponentJoint< RigidJoint >( RigidBody, CubeB, 
        //         translate( { double(length-1), 0, 0 } ) * rotate( Angle::pi, { 0, 0, 1 } ) ),
        // };

        return joints;
    }

    static std::vector< ComponentJoint > _initJoints( int width, int height ){
        if ( width == 1 || height == 1 ) return _initJoints( std::max( width, height ) );
        assert( width > 1 && height > 1 && "Width and height of a support must be at least 2" );
        
        std::vector< ComponentJoint > joints;
        int roficoms = roficomCount( width, height );
        int compCount = roficoms + 2 * cubeCount( width, height ) - 1;

        joints.push_back( makeComponentJoint< RigidJoint >( roficoms, 0, rotate( Angle::pi / 2, { 0, 0, 1 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( roficoms, 1, rotate( Angle::pi, { 0, 0, 1 } ) ) );
        int currRoficom = 2;
        for ( int currComp = roficoms; currComp < compCount - 1; currComp += 2 )
        {
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currComp + 1, // Cube to cylinder
                translate( { -0.5, 0.5, 0 } ) * rotate( Angle::pi / 4, { 0, 0, -1 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp + 1, currComp + 2, // Cylinder to cube
                rotate( Angle::pi / 4, { 0, 0, 1 } ) * translate( { -0.5, 0.5, 0 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom, rotate( Angle::pi / 2, { 0, -1, 0 } ) ) );
            joints.push_back( makeComponentJoint< RigidJoint >( currComp, currRoficom + 1, rotate( Angle::pi / 2, { 0, 1, 0 } ) ) );
            currRoficom += 2;
        }
        assert( currRoficom == roficoms - 4 );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom, rotate( Angle::pi / 2, { 0, -1, 0 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 1, rotate( Angle::pi / 2, { 0, 1, 0 } ) ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 2, identity ) );
        joints.push_back( makeComponentJoint< RigidJoint >( compCount - 1, currRoficom + 3, rotate( Angle::pi / 2, { 0, 0, -1 } ) ) );

        // std::vector< ComponentJoint > joints = {
        //     makeComponentJoint< RigidJoint >( CubeA, 0, identity ), 
        //     makeComponentJoint< RigidJoint >( CubeA, 1, rotate( Angle::pi / 2, { 0, -1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeA, 2, rotate( Angle::pi / 2, { 0, 1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeA, 3, rotate( Angle::pi / 2, { 0, 0, 1 } ) ),
    
        //     makeComponentJoint< RigidJoint >( CubeB, 4, identity ),
        //     makeComponentJoint< RigidJoint >( CubeB, 5, rotate( Angle::pi / 2, { 0, -1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 6, rotate( Angle::pi / 2, { 0, 1, 0 } ) ),
        //     makeComponentJoint< RigidJoint >( CubeB, 7, rotate( Angle::pi / 2, { 0, 0, 1 } ) ),

        //     makeComponentJoint< RigidJoint >( RigidBody, CubeA, identity ), // Center is in CubeA
        //     makeComponentJoint< RigidJoint >( RigidBody, CubeB, 
        //         translate( { double(width-1), double(height-1), 0 } ) * rotate( Angle::pi, { 0, 0, 1 } ) ),
        // };

        return joints;
    }

  public:
    const int width  = 0;
    const int height = 0;

    Support( int id, int width, int height ) : 
        Module( ModuleType::Support, 
            _initComponents( width, height ), 
            roficomCount( width, height ), 
            _initJoints( width, height ), id ),
        width( width ), height( height ) {}

    ATOMS_CLONEABLE( Support );
};


} // namespace rofi::configuration
