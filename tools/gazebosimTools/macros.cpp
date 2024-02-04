#include <cassert>
#include <future>
#include <iostream>

#include <atoms/units.hpp>

#include "macros.hpp"

using namespace rofi::hal;

static constexpr int A_X = 0;
static constexpr int ApX = 1;
static constexpr int A_Z = 2;
static constexpr int B_X = 3;
static constexpr int BpX = 4;
static constexpr int B_Z = 5;

namespace rofi::macro {

void setJointPosWait( Joint joint, float pos, float speedMultiplier )
{
    auto jointPromise = std::promise< void >();

    setJointPos( joint, pos, [ &jointPromise ]{ jointPromise.set_value(); }, speedMultiplier );

    jointPromise.get_future().get();
}

void extend( Connector connector )
{
    if ( auto state = connector.getState(); state.position == ConnectorPosition::Extended ) {
        std::cout << "Connector already extended" << std::endl;
        return;
    }

    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( Connector, ConnectorEvent event ) {
        if ( event == ConnectorEvent::Connected ) {
            std::call_once( onceFlag, [ & ]() { 
                std::cout << "Connected modules" << std::endl;
            } );
        }
    } );

    connector.connect();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );
}

void retract( Connector connector )
{
    if ( auto state = connector.getState(); state.position == ConnectorPosition::Retracted ) {
        std::cout << "Connector already retracted" << std::endl;
        return;
    }

    // std::promise< void > prom;
    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( Connector, ConnectorEvent event ) {
        if ( event == ConnectorEvent::Disconnected ) {
            std::call_once( onceFlag, [ & ]() {  
                std::cout << "Disconnected modules" << std::endl; 
                // prom.set_value();
            } );
        } 
    } );

    connector.disconnect();
    // prom.get_future().get();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );

}

bool tryConnect( Connector connector )
{
    if ( connector.getState().connected ) return true;

    retract( connector );
    {
        std::promise< void > waitPromise;
        RoFI::postpone( 100, [ &waitPromise ] { waitPromise.set_value(); } );
        waitPromise.get_future().get();
    }
    extend( connector );
    {
        std::promise< void > waitPromise;
        RoFI::postpone( 100, [ &waitPromise ] { waitPromise.set_value(); } );
        waitPromise.get_future().get();
    }
    
    return connector.getState().connected;
}

void resetJointsWait( RoFI& rofiMod )
{
    assert( rofiMod.getDescriptor().jointCount == 3 && rofiMod.getDescriptor().connectorCount == 6 );
    std::promise< void > promA, promB, promC;

    setJointPos( rofiMod.getJoint( 0 ), 0, [ &promA ]{ promA.set_value(); } );
    setJointPos( rofiMod.getJoint( 1 ), 0, [ &promB ]{ promB.set_value(); } );
    setJointPos( rofiMod.getJoint( 2 ), 0, [ &promC ]{ promC.set_value(); } );

    promA.get_future().get();
    promB.get_future().get();
    promC.get_future().get();
}

bool moveWait( RoFI& rofiMod, int bottomConnId, bool forward )
{
    if ( rofiMod.getDescriptor().jointCount != 3 || rofiMod.getDescriptor().connectorCount != 6 )
    {
        std::cout << "Selected module " << rofiMod.getId() << " is not a universal module" << std::endl;
        return false;
    }
    if ( bottomConnId != A_Z && bottomConnId != B_Z && bottomConnId != ApX && bottomConnId != B_X )
    {
        std::cout << "Selected connector " << bottomConnId << " is not one of A-Z, B-Z, ApX, B_X" << std::endl;
        return false;
    }

    // topConnId == A_Z <=> bottomConnId == B_Z
    int topConnId;
    if ( bottomConnId == A_Z || bottomConnId == B_Z )
        topConnId = A_Z + B_Z;
    else topConnId = ApX + B_X;
    topConnId -= bottomConnId;
        
    Connector bottomConn = rofiMod.getConnector( bottomConnId );

    // If bottom connector is not connected, attempt connection
    // if ( !bottomConn.getState().connected )
    // {
    //     if ( !tryConnect( bottomConn ) )
    //     {
    //         std::cout << "Selected bottom connector " << bottomConnId << " cannot be attached" << std::endl;
    //         return;
    //     }
    // }

    // Disconnect all other connections
    // for ( int connId = 0; connId < rofiMod.getDescriptor().connectorCount; ++connId )
    //     if ( connId != bottomConnId && rofiMod.getConnector( connId ).getState().connected )
    //         retract( rofiMod.getConnector( connId ) );

    // Straighten up the module
    resetJointsWait( rofiMod );

    // Bend top and bottom
    {
        std::promise< void > promA, promB;

        float pos = ((topConnId > 2) == forward ? 90_deg : -90_deg).rad();
        int attachedJointId = topConnId > 2 ? 0 : 1;
        int topJointId = 1 - attachedJointId;
        
        setJointPos( rofiMod.getJoint( attachedJointId ), pos, [ &promA ]{ promA.set_value(); } );
        setJointPos( rofiMod.getJoint( topJointId ), pos, [ &promB ]{ promB.set_value(); } );

        promA.get_future().get();
        promB.get_future().get();
    }

    // Connect top connector
    if ( !tryConnect( rofiMod.getConnector( topConnId ) ) )
    {
        std::cout << "Cannot move in selected direction, stopping" << std::endl;
        resetJointsWait( rofiMod );
        return false;
    }

    // Wait 100 ms
    {
        std::promise< void > waitPromise;
        RoFI::postpone( 100, [ &waitPromise ] { waitPromise.set_value(); } );
        waitPromise.get_future().get();
    }

    // Disconnect bottom connector
    retract( rofiMod.getConnector( bottomConnId ) );

    // Wait 100 ms
    {
        std::promise< void > waitPromise;
        RoFI::postpone( 100, [ &waitPromise ] { waitPromise.set_value(); } );
        waitPromise.get_future().get();
    }

    // Straighten top and bottom
    resetJointsWait( rofiMod );
    return true;
}

} // namespace rofi::macro

// case CommandType::Move: // move modId [iterations=1] [forward=true]
// {
//     int modId = std::stoi( params[0] );
//     int iterations = 1;
//     if ( params.size() >= 2 ) iterations = std::stoi( params[1] );
//     bool goForward = true;
//     if ( params.size() >= 3 ) std::istringstream(params[2]) >> std::boolalpha >> goForward; 
//     // std::cout << "modId: " << modId << ", bottomConnId: " << bottomConnId << std::endl;

//     if ( !modules.contains( modId ) ) { 
//         std::cout << "module " << modId << " not locked" << std::endl;
//         return false;
//     }

//     auto [ _, reqMod ] = *(modules.find( modId ));
//     assert( reqMod.getId() == modId ); 

//     int attachedConnId = reqMod.getConnector( macro::A_Z ).getState().connected ? macro::A_Z : macro::B_Z;
//     for ( int j = 0; j < iterations; ++j ) 
//     {
//         if( !macro::moveWait( reqMod, attachedConnId, goForward ) ) break;
//         attachedConnId = macro::A_Z + macro::B_Z - attachedConnId; // switch attached connector
//     }
//     return true;
// } 
// case CommandType::SideMove: // smove modId [iterations=1] [forward=true]
// {
//     int modId = std::stoi( params[0] );
//     int iterations = 1;
//     if ( params.size() >= 2 ) iterations = std::stoi( params[1] );
//     bool goForward = true;
//     if ( params.size() >= 3 ) std::istringstream(params[2]) >> std::boolalpha >> goForward; 
//     // std::cout << "modId: " << modId << ", bottomConnId: " << bottomConnId << std::endl;

//     if ( !modules.contains( modId ) ) { 
//         std::cout << "module " << modId << " not locked" << std::endl;
//         return false;
//     }

//     auto [ _, reqMod ] = *(modules.find( modId ));
//     assert( reqMod.getId() == modId ); 

//     int attachedConnId = reqMod.getConnector( macro::ApX ).getState().connected ? macro::ApX : macro::B_X;
//     for ( int j = 0; j < iterations; ++j ) 
//     {
//         if( !macro::moveWait( reqMod, attachedConnId, goForward ) ) break;
//         attachedConnId = macro::ApX + macro::B_X - attachedConnId; // switch attached connector
//     }
//     return true;
// }
