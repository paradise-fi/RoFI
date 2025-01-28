#pragma once

#include "lwip++.hpp"
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>

#include <enums.hpp>
#include <atoms/util.hpp>

#include <vector>
#include <map>
#include <set>
#include <utility>


namespace rofi::net {
    using namespace rofi::leadership;
    class EchoElection : public Protocol {
        struct ConnectionInfo {
            bool receivedElection = false;
            bool receivedLeader = false;
            bool initiateReceived = false;
        };

        std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
        std::set< std::string > _interfaceWithCb;

        std::vector< std::pair< ConfigAction, ConfigChange > > _confChanges;

        Ip6Addr _id;
        Ip6Addr _leaderId;
        Ip6Addr _currentWaveId;

        Interface::Name _parent = "rl0";
        Interface::Name _connected = "rl0";

        MessageType _messageType;

        std::map< Interface::Name, ConnectionInfo > _connections;
        std::function< void ( Ip6Addr, ElectionStatus ) >& _electionChangeCallback;

        void _resetReceived() {
            for ( const auto& [ key, _ ] : _connections ) {
                _connections[ key ].receivedLeader = false;
                _connections[ key ].receivedElection = false;
            }
        }

        /** @param election determines if we are checking for all election messages received or all initiate messages received. */
        bool _allReceived( bool election ) {
            for ( const auto& [ key, _ ] : _connections ) {
                if ( election && !_connections[ key ].receivedElection ) {
                    return false;
                }
                if ( !election && !_connections[ key ].initiateReceived ) {
                    return false;
                }
            }
            return true;
        }

        bool _leaderReceived( bool all ) {
            for ( const auto& [ key, _ ] : _connections ) {
                if ( !all && _connections[ key ].receivedLeader ) {
                    return false;
                }
                if ( all && !_connections[ key ].receivedLeader ) {
                    return false;
                }
            }
            return true;
        }

        bool _onElectionMessage( const std::string& interfaceName, Ip6Addr waveId ) {
            if ( waveId > _currentWaveId ) {
                // Restart occured.
                if ( _currentWaveId == _id ) {
                    _electionChangeCallback( _id, ElectionStatus::UNDECIDED );
                    _parent = "rl0";
                    _resetReceived();
                    _messageType = MessageType::ELECTION_MESSAGE;
                    _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                    return true;
                }
                return false;
            }

            _connections[ interfaceName ].receivedElection = true;

            if ( waveId == _currentWaveId ) {
                if ( !_allReceived( true ) ) {
                    return false;
                }

                // This means that the node was elected.
                if ( waveId == _id ) {
                    _electionChangeCallback( _id, ElectionStatus::LEADER );
                    _leaderId = _id;
                    _parent = "rl0";
                    _connections[ interfaceName ].receivedLeader = true;
                    _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                    _messageType = MessageType::LEADER_MESSAGE;
                    return true;
                }
                _messageType = MessageType::ELECTION_MESSAGE;
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                return true;
            }

            // waveId < _currentWaveId, restart.
            if ( _messageType == MessageType::LEADER_MESSAGE ) {
                _electionChangeCallback( _leaderId, ElectionStatus::UNDECIDED );
            }
            
            _currentWaveId = waveId;
            _parent = interfaceName;
            _resetReceived();
            _connections[ interfaceName ].receivedElection = true;
            _messageType = MessageType::ELECTION_MESSAGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _onLeaderMessage( const std::string& interfaceName, Ip6Addr waveId ) {
            // This is the first leader message.
            if ( ( _leaderReceived( false ) && waveId != _id ) || waveId < _leaderId ) {
                _electionChangeCallback( waveId, ElectionStatus::FOLLOWER );
                _connections[ interfaceName ].receivedLeader = true;
                _leaderId = waveId;
                _currentWaveId = _id;
                _parent = interfaceName;
                _messageType = MessageType::LEADER_MESSAGE;
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                return true;
            }

            return false;
        }

        bool _onInitiateMessage( const std::string& interfaceName ) {
            _connections[ interfaceName ].initiateReceived = true;
            if ( _managedInterfaces.size() == rofi::hal::RoFI::getLocalRoFI().getDescriptor().connectorCount + 1 && _allReceived( false ) ) {
                _messageType = MessageType::ELECTION_MESSAGE;
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                return true;
            }

            if ( _messageType != MessageType::INITIATE_MESSAGE ) {
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                return true;
            }

            return false;
        }

        bool _onConnectMessage( const std::string& interfaceName, Ip6Addr otherLeader ) {
            if ( otherLeader < _leaderId ) {
                return _onLeaderMessage( interfaceName, otherLeader );
            }

            if ( otherLeader == _leaderId ) {
                return false;
            }

            if ( _id == _leaderId ) {
                _electionChangeCallback( _leaderId, ElectionStatus::CHANGED_FOLLOWERS );
                return false;
            }

            _messageType = MessageType::FOLLOWER_CHANGE_MESSAGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
            return true;
        }

        // Sending an update about follower change back to the leader.
        bool _onConnectionEstablishedMessage() {
            if ( _leaderId == _id ) {
                _electionChangeCallback( _leaderId, ElectionStatus::CHANGED_FOLLOWERS );
                return false;
            }

            _messageType = MessageType::FOLLOWER_CHANGE_MESSAGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _disconnectActions( const Interface& interface ) {
            // For some reason, the disconnect interface event is triggered twice. This is a safeguard against 
            // restarting the same process twice.
            if ( _connections.find( interface.name() ) == _connections.end() ) {
                return false;
            }

            _connections.erase( interface.name() );
            if ( interface.name() == _parent ) {
                _parent = "rl0";
                _resetReceived();
                _electionChangeCallback( _id, ElectionStatus::UNDECIDED );
                if ( _connections.size() == 0 ) {
                    return _onElectionMessage( interface.name(), _id );
                }
                _messageType = MessageType::ELECTION_MESSAGE;
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                return true;
            }

            if ( _id == _leaderId ) {
                _electionChangeCallback( _leaderId, ElectionStatus::CHANGED_FOLLOWERS );
                return false;
            }

            _messageType = MessageType::FOLLOWER_CHANGE_MESSAGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _connectActions( const Interface& interface ) {
            _connections[ interface.name() ].receivedElection = true;
            _messageType = MessageType::CONNECT_MESSAGE;
            _connected = interface.name();
            _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
            return true;
        }

    public:
        /** Instantiates the wave-based election protocol. This protocol must manage every 
         * module interface to start, and it is not recommended to change the number of managed 
         * interfaces, unless the protocol is being completely disabled.
         * @param address The address this module will use for identification within the protocol.
         * @param callback Callback function that informs the user of changes in the election. 
         *           The Ip6Addr parameter informs of the leader's address, while ElectionStatus
         *           informs of the status of the election. Further work should be delayed until
         *           the module is out of the UNDECIDED status.
        */
        EchoElection( const Ip6Addr& address, std::function< void( Ip6Addr, ElectionStatus ) > callback )
        : _id( address ), _leaderId( address ), _currentWaveId( address ), _electionChangeCallback( callback ) {
            _parent = "rl0";
            _messageType = MessageType::INITIATE_MESSAGE;
        }

        virtual ~EchoElection() = default;

        virtual bool onMessage( const std::string& interfaceName,
                               rofi::hal::PBuf packetWithHeader ) override {
            auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
            
            MessageType type = as< MessageType >( packet.payload() );
            Ip6Addr waveId = as< Ip6Addr >( packet.payload() + sizeof( MessageType ) );

            switch ( type ) {
                case MessageType::LEADER_MESSAGE:
                    return _onLeaderMessage( interfaceName, waveId );
                case MessageType::ELECTION_MESSAGE:
                    return _onElectionMessage( interfaceName, waveId );
                case MessageType::INITIATE_MESSAGE:
                    return _onInitiateMessage( interfaceName );
                case MessageType::CONNECT_MESSAGE:
                    return _onConnectMessage( interfaceName, waveId );
                case MessageType::FOLLOWER_CHANGE_MESSAGE:
                    return _onConnectionEstablishedMessage();
            }

            std::cout << "Wave Election: Unexpected Behavior.\n";
            return false;
        }

        virtual bool afterMessage( const Interface& interface, 
                                   std::function< void ( PBuf&& ) > fun, void* /* args */ ) override {
            // Mostly to prevent the pointless sending of extra Intitialization messages and creating messages with no connector to send them to.
            if ( ( _messageType == MessageType::INITIATE_MESSAGE && 
                 _managedInterfaces.size() != rofi::hal::RoFI::getLocalRoFI().getDescriptor().connectorCount + 1 )
                 || ! const_cast< Interface& >( interface ).isConnected() ) {
                return false;
            }
            
            if ( _messageType == MessageType::ELECTION_MESSAGE ) {
                // In this situation, the echo algorithm is reporting back to its parent.
                if (  _allReceived( true ) && interface.name() != _parent ) {
                    return false;
                }

                if ( !_allReceived( true ) && interface.name() == _parent ) {
                    return false;
                }
            }

            if ( _messageType == MessageType::CONNECT_MESSAGE && interface.name() != _connected ) {
                return false;
            }

            if ( _messageType == MessageType::FOLLOWER_CHANGE_MESSAGE && interface.name() != _parent ) {
                return false;
            }

            PBuf packet = PBuf::allocate( sizeof( MessageType ) + Ip6Addr::size() );
            as< MessageType >( packet.payload() ) = _messageType;
            if ( _messageType == MessageType::ELECTION_MESSAGE || _messageType == MessageType::INITIATE_MESSAGE ) {
                as< Ip6Addr >( packet.payload() + sizeof( MessageType ) ) = _currentWaveId;
            } else {
                as< Ip6Addr >( packet.payload() + sizeof( MessageType ) ) = _leaderId;
            }
            
            fun( std::move( packet ) );
            return false;
        }

        virtual bool hasConfigUpdates() const override { return!_confChanges.empty(); }

        virtual std::vector< std::pair< ConfigAction, ConfigChange > > getConfigUpdates() const {
            return _confChanges;
        }

        virtual void clearUpdates() { _confChanges.clear(); }

        virtual bool addInterface( const Interface& interface ) {
            if ( manages( interface ) ) {
                return false;
            }
            _managedInterfaces.push_back( std::reference_wrapper( interface ) );
            if ( const_cast< Interface& >( interface ).isConnected() && _connections.find( interface.name() ) == _connections.end() ) {
                _connections[ interface.name() ].receivedElection = false;
                // For the rare case the user decides to remove the interface and re-add it. Should not be done, however.
                if ( _messageType == MessageType::LEADER_MESSAGE ) {
                    return _connectActions( interface );
                }
            }
            return false;
        }

        virtual bool removeInterface( const Interface& interface ) {
            auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end()
                                , [ &interface ]( const auto& i ) { return interface == i; } );
            if ( it == _managedInterfaces.end() )
                return false;

            // In theory, a user shouldn't ever remove an interface 
            // except for when removing the protocol.
            _disconnectActions( interface );
            std::swap( *it, _managedInterfaces.back() );
            _managedInterfaces.pop_back();
            return true;
        }

        virtual bool onInterfaceEvent( const Interface& interface, bool connected ) override {
            if ( !connected ) {
                return _disconnectActions( interface );
            }

            if ( connected ) {
                return _connectActions( interface );
            }

            return false;
        }

        virtual bool manages( const Interface& interface ) const override {
            return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
                return interface == i;
            } );
        }

        virtual Ip6Addr address() const override { return Ip6Addr( "ff02::ea:ea" ); }

        virtual std::string name() const override { return "echo-election"; }

        virtual std::string info() const override {
            std::string str = Protocol::info();
            std::stringstream ss;
            ss << "; leader id: " << _leaderId;
            return str  + ss.str();
        }
    };
}