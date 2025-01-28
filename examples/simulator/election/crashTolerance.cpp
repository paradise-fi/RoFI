#include <iostream>
#include <lwip++.hpp>
#include <lwip/udp.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <networking/networkManager.hpp>
#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>
#include <invitation.hpp>
#include <LRElect.hpp>
#include <atoms/units.hpp>
#include <random>

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

int NODE_STATE_CHANGE_CHANCE = -1;

Ip6Addr createAddress( int id ) {
    std::stringstream ss;
    ss << "fc07:0:0:";
    ss << id;
    ss << "::1";

    return Ip6Addr( ss.str() );
}

PBuf calcTask() {
    std::cout << "Calctask\n";
    auto task = PBuf::allocate(sizeof( int ) * 6 );
    int size = 5;
    as< int >( task.payload() ) = size;
    auto* data = task.payload() + sizeof( int );
    for ( int i = 0; i < 5; i++ ) {
        as< int >( data ) = i;
        data = data + sizeof( int );
    }
    return task;
}

void getTask ( void* data ) {
    int size = as< int >( data );
    std::cout << "Size is: " << size << "\n";
    std::cout << "Task: ";
    
    for ( int i = 1; i <= size; i++ ) {
        std::cout << as< int >( data + i * ( sizeof( int ) ) ) << " ";
    }
    std::cout << "\n";
}


bool workStop = false;

void stopWork() {
    std::cout << "Stopping work\n";
    workStop = true;
}

void changeConn( bool connect ) {
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

void testInvitation( NetworkManager& netmg, int id, Ip6Addr& addr ) {
    auto proto = netmg.addProtocol( SimpleReactive() );
    netmg.setProtocol( *proto );

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 100);

    std::vector< Ip6Addr > addresses;
    addresses.push_back( ( createAddress( 1 ) ) );
    addresses.push_back( ( createAddress( 2 ) ) );
    addresses.push_back( ( createAddress( 3 ) ) );
    addresses.push_back( ( createAddress( 4 ) ) );
    addresses.push_back( ( createAddress( 5 ) ) );
    // addresses.push_back( ( createAddress( 6 ) ) );
    // addresses.push_back( ( createAddress( 7 ) ) );


    InvitationElection election( id, addr, 7776, addresses, calcTask, getTask, stopWork, 1, 3 );
    election.setUp();
    election.start();

    NetworkManagerCli netcli( netmg );
    std::string line;
    while ( std::getline( std::cin, line ) ) {
        if ( line.empty() ) {
            std::cout << ">> ";
            continue;
        } else if ( line == "end" ) {
            return;
        } else if ( line == "leader" || line == "lead" ) {
            std::cout << "My (" << addr << ") leader: " << election.getLeader().first << "\n";
            if ( election.getLeader().second ) {
                std::cout << "The leader election of this leader finished and tasks have been redistributed.\n";
            }
        }

        try {
            if ( line == "connect" || line == "disconnect" ) {
                changeConn( line == "connect" );
            } else if ( netcli.command( line ) ) {}
        } catch ( const std::exception& exc ) {
            std::cout << "Bad input: " << exc.what() << std::endl;
        }

        std::cout << ">> ";
    }
}

void testLR( NetworkManager& net, int id, Ip6Addr& addr ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, 100);

    LRElect election( net, addr, 5 );
    election.start( id );
    election.start( id );
    
    NetworkManagerCli netcli( net );
    std::string line;
    while ( std::getline( std::cin, line ) ) {
        if ( line.empty() ) {
            std::cout << ">> ";
            continue;
        } else if ( line == "end" ) {
            return;
        } else if ( line == "leader" || line == "lead" ) {
            std::cout << "My (" << addr << ") leader: " << election.getLeader() << "\n";
        }

        try {
            if ( line == "connect" || line == "disconnect" ) {
                changeConn( line == "connect" );
            } else if ( netcli.command( line ) ) {}
        } catch ( const std::exception& exc ) {
            std::cout << "Bad input: " << exc.what() << std::endl;
        }

        std::cout << ">> ";
    }
}

void testTolerant( bool invitationTest ) {
    std::cout << "Starting crash tolerant election test\n";
    tcpip_init( nullptr, nullptr );

    int id = RoFI::getLocalRoFI().getId();
    std::cout << "This module is: " << id << "\n";

    NetworkManager net( RoFI::getLocalRoFI() );

    Ip6Addr addr = createAddress( id );

    net.addAddress( addr, 80, net.interface( "rl0" ) );
    std::cout << "Current Address: " << net.interface( "rl0" ).getAddress().front().first << "\n";
    net.setUp();

    if ( invitationTest ) {
        testInvitation( net, id, addr );
        return;
    }

    testLR( net, id, addr );
}