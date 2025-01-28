#include <iostream>
#include <lwip++.hpp>
#include <echo.hpp>
#include <traverseElect.hpp>
#include <networking/networkManagerCli.hpp>

using namespace rofi::hal;
using namespace rofi::net;

void electionStatusUpdate( bool elected, ElectionStatus nodeStatus ) {
    if ( elected ) {
        std::cout << "Election finished.\n";
    }
    std::cout << "NodeStatus is : ";
    if ( nodeStatus == ElectionStatus::UNDECIDED ) {
        std::cout << "Undecided\n";
        return;
    }
    if ( nodeStatus == ElectionStatus::CHANGED_FOLLOWERS ) {
        std::cout << "A possible change in follower configuration was registered.\n";
        return;
    } 
    std::cout << ( ( nodeStatus == ElectionStatus::FOLLOWER ) ? "Follower\n" : "Leader\n" );
}

void electionResult( Ip6Addr leaderAddr, ElectionStatus status ) {
    
}

Ip6Addr getAddress( int id ) {
    std::stringstream ss;
    ss << "fc07:0:0:";
    ss << id;
    ss << "::1";

    return Ip6Addr( ss.str() );
}

void changeConnection( bool connect ) {
    int connector;
    std::cout << "Connector to " << ( ( connect ) ? "connect" : "disconnect" ) << "(0 - 5):";
    std::cin >> connector;

    auto rofi = RoFI::getLocalRoFI();

    if ( connector >= 0 && connector < 6 ) {
        if ( connect ) {
            rofi.getConnector( connector ).connect();
        } else {
            rofi.getConnector( connector ).disconnect();
        }
    }
}

void printConnected( NetworkManager& net ) {
    std::cout << "Connected interfaces: ";
    for ( const Interface& interface : net.interfaces() ) {
        if ( const_cast< Interface& >( interface ).isConnected() ) {
            std::cout << interface.name() << " ";
        }
    }
    std::cout << "\n";
}

void testProtocol( bool echoTest ) {
    tcpip_init( nullptr, nullptr );
    auto localRoFI = RoFI::getLocalRoFI();
    int id = localRoFI.getId();
    std::cout << "Testing protocol for ID: " << id << "\n";
    NetworkManager net( localRoFI );
    NetworkManagerCli netcli( net );
    net.addAddress( getAddress( id ), 80, net.interface( "rl0" ) );
    net.setUp();

    Ip6Addr addr = "fc07:b::a"_ip;
    Ip6Addr* leader = &addr;
    bool work = false;
    
    if ( echoTest ) {
        Protocol* protocol = net.addProtocol( EchoElection( getAddress( id ), [ leader, work ]( Ip6Addr leadAddr, ElectionStatus status ) {
            *leader = leadAddr;
            if ( status == ElectionStatus::LEADER ) {
                std::cout << "I am leader\n";
            }

            if ( status == ElectionStatus::FOLLOWER ) {
                std::cout << "I am a follower of " << *leader << "\n";
            }

            if ( status == ElectionStatus::CHANGED_FOLLOWERS ) {
                std::cout << "A follower change occured\n";
            }

            if ( status == ElectionStatus::UNDECIDED ) {
                std::cout << "Election not decided yet.\n";
            }
        } ) );
        net.setProtocol( *protocol );
    } else {
        Protocol* protocol = net.addProtocol( TraverseElection( getAddress( id ), [ leader, work ]( Ip6Addr leadAddr, ElectionStatus status ) {
            *leader = leadAddr;
            if ( status == ElectionStatus::LEADER ) {
                std::cout << "I am leader\n";
            }

            if ( status == ElectionStatus::FOLLOWER ) {
                std::cout << "I am a follower of " << *leader << "\n";
            }

            if ( status == ElectionStatus::CHANGED_FOLLOWERS ) {
                std::cout << "A follower change occured\n";
            }

            if ( status == ElectionStatus::UNDECIDED ) {
                std::cout << "Election not decided yet.\n";
            }
        } ) );
        net.setProtocol( *protocol );
    }

    std::string line;
    while ( std::getline( std::cin, line ) ) {
        if ( line.empty() ) {
            std::cout << ">> ";
            continue;
        } else if ( line == "end" ) {
            break;
        }

        try { 
            if ( line == "connect" || line == "disconnect" ) {
                changeConnection( line == "connect" );
            } else if ( line == "connected" ) {
                printConnected( net );
            } else if ( netcli.command( line ) ) {} 
        } catch ( const std::exception& exc ) {
            std::cout << "Bad input: " << exc.what() << std::endl;
        }

        std::cout << ">> ";
    }
}