#pragma once

#include "lwip++.hpp"
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>

#include <atoms/util.hpp>

#include <vector>
#include <map>
#include <set>
#include <utility>

namespace rofi::net {
    class TraverseElection : public Protocol {
        enum TokenType {
            INITIATE,
            ANNEXING,
            CHASING,
            LEADER,
            CHANGE,
            CONNECT,
        };

        struct Token {
            TokenType type;
            int phase;
            Ip6Addr identity = Ip6Addr( "::" );
            int hopCount;
        };

        struct ChannelInfo {
            bool initiated;
            bool sent;
        };

        std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
        std::set< std::string > _interfaceWithCb;
        std::vector< std::pair< ConfigAction, ConfigChange > > _confChanges;

        std::map< Interface::Name, ChannelInfo > _channels;
        Interface::Name _parent = "rl0";
        Interface::Name _next = "rl0";
        Interface::Name _connection;

        const Ip6Addr& _id;
        Ip6Addr _leaderId;

        int _currentPhase;
        int _chasedPhase;
        
        int _maxHops;
        
        Ip6Addr _tokenId;
        Ip6Addr _waitingTokenId;

        TokenType _sendType;
        std::function< void ( Ip6Addr, ElectionStatus ) >& _electionChangeCallback;

        bool _allInitiated( ) {
            for ( const auto& [ _ , value ] : _channels ) {
                if ( !value.initiated ) {
                    return false;
                }
            }
            return true;
        }

        void _determineNextNode( ) {
            for ( const auto& [ key, _ ] : _channels ) {
                if ( key != _parent && !_channels[ key ].sent ) {
                    _next = key;
                    return;
                }
            }
            // We need to make sure "rl0" is not added to _channels,
            // hence why we make sure it isn't _parent here.
            if ( _parent != "rl0" && !_channels[ _parent ].sent ) {
                _next = _parent;
                return;
            }
            // The traversal algorithm ends.
            _next = "rl0";
        }

        void _resetSent() {
            for ( const auto& [ key, _ ] : _channels ) {
                _channels[ key ].sent = false;
            }
        }

        void _resetTraversal( Ip6Addr tokenId, int phase, int hops ) {
            _tokenId = tokenId;
            _currentPhase = phase;
            _chasedPhase = -1;
            _maxHops = hops;
            _waitingTokenId = Ip6Addr( "::" );
        }

        bool _leaderActions( Ip6Addr id, const std::string& interfaceName ) {
            if ( id == _id ) {
                _electionChangeCallback( _leaderId, ElectionStatus::LEADER );
            } else {
                _electionChangeCallback( _leaderId, ElectionStatus::FOLLOWER );
            }

            _sendType = TokenType::LEADER;
            _leaderId = id;
            _resetTraversal( Ip6Addr( "::" ), -1, -1 );
            _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        // Note that onAnnexingToken could be merged with onChasingToken, but this is more
        // readable, as it provides clear structure for how each token type is handled.
        bool _onAnnexingToken( const std::string& interfaceName,
                               Token receivedToken ) {
            if ( _parent == "rl0" ) {
                _parent = interfaceName;
                _electionChangeCallback( _leaderId, ElectionStatus::UNDECIDED );
            }

            // Tokens of lower phases are redundant and thus killed off.
            if ( receivedToken.phase < _currentPhase ) {
                return false;
            }

            // Indication that a restart occured.
            if ( _sendType == TokenType::LEADER || _sendType == TokenType::CHANGE ) {
                _resetSent();
            }

            if ( _maxHops < receivedToken.hopCount ) {
                _maxHops = receivedToken.hopCount;
            }

            // A token of larger phase appears, the node has to reset to respect new traversal.
            if ( receivedToken.phase > _currentPhase ) {
                _resetTraversal( receivedToken.identity, receivedToken.phase, receivedToken.hopCount );
                _parent = interfaceName;
                _resetSent();
                _sendType = TokenType::ANNEXING;
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                _determineNextNode(); // Since this is when a 're-initialization' occurs, we should never receive 'rl0' as next here.
                return true;
            }

            // A token is waiting here, met by another token. We combine them to a higher phase token.
            if ( _waitingTokenId != Ip6Addr( "::" ) ) {
                _resetTraversal( _id, _currentPhase + 1, -1 );
                _parent = "rl0";
                _resetSent();
                _sendType = TokenType::ANNEXING;
                _determineNextNode();
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            }

            // The node is annexed, any lower phase waiting tokens are deleted.
            if ( receivedToken.identity == _tokenId 
                 && _chasedPhase != receivedToken.phase ) {
                _waitingTokenId = Ip6Addr( "::" );
                _determineNextNode();
                if ( _next == "rl0" ) {
                    // This node won the election. It will now inform its followers and declare itself a leader after
                    // the information round is finished.
                    return _onLeaderToken( "rl0", _id );
                }
                _sendType = TokenType::ANNEXING;
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            }

            if ( _chasedPhase == receivedToken.phase 
                || _tokenId > receivedToken.identity ) {
                _waitingTokenId = receivedToken.identity;
                return false;
            }

            _chasedPhase = _currentPhase;
            _sendType = TokenType::CHASING;
            _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _onChasingToken( const std::string& interfaceName,
                              Token receivedToken ) {
            // No chance of this token completing traversal.
            if ( receivedToken.phase < _currentPhase ) {
                return false;
            }

            // The chase continues.
            if ( receivedToken.phase == _currentPhase 
                 && receivedToken.identity == _tokenId  
                 && _maxHops > receivedToken.hopCount 
                 && _chasedPhase != receivedToken.phase 
                 && _waitingTokenId == Ip6Addr( "::" ) ) {
                _chasedPhase = receivedToken.phase;
                _sendType = TokenType::CHASING;
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            } 

            // The chasing token catches up, the two tokens merge.
            if ( receivedToken.phase == _currentPhase && _waitingTokenId != Ip6Addr( "::" ) ) {
                _resetTraversal( _id, _currentPhase + 1, -1 );
                _parent = "rl0";
                _sendType = TokenType::ANNEXING;
                _resetSent();
                _determineNextNode();
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            }

            // The token waits here.
            _waitingTokenId = receivedToken.identity;
            return false;
        }

        bool _onInitiateToken( const std::string& interfaceName ) {
            _channels[ interfaceName ].initiated = true;
            if ( _managedInterfaces.size() == rofi::hal::RoFI::getLocalRoFI().getDescriptor().connectorCount + 1 
                 && _allInitiated() && _currentPhase == -1 ) {
                _sendType = TokenType::ANNEXING;
                _determineNextNode();
                _currentPhase = 0;
                _tokenId = _id;
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            }

            if ( _next == interfaceName ) {
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
                return true;
            }

            return false;
        }

        bool _onLeaderToken( const std::string& interfaceName,
                             Ip6Addr tokenId ) {
            // This is the first time a leader token is forward to this node.
            if ( _sendType != TokenType::LEADER ) {
                _resetSent();
                _parent = interfaceName;
            }
            _sendType = TokenType::LEADER;
            _leaderId = tokenId;
            _determineNextNode();
            if ( _next == "rl0" || _next == _parent ) {
                return _leaderActions( tokenId, interfaceName );
            }
            _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _onChangeToken( const std::string& interfaceName ) {
            if ( _leaderId == _id ) {
                _electionChangeCallback( _leaderId, ElectionStatus::CHANGED_FOLLOWERS );
                return false;
            }
            _sendType = TokenType::CHANGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _onConnectToken( const std::string& interfaceName, Ip6Addr otherLeader ) {
            if ( otherLeader == _leaderId ) {
                _sendType = TokenType::LEADER;
                return false;
            }
            _resetSent();
            _sendType = TokenType::ANNEXING;
            _parent = "rl0";
            _determineNextNode();
            if ( _next == "rl0" ) {
                return _leaderActions( _id, interfaceName );
            } 
            _tokenId = _id;
            _currentPhase = 0;
            _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _disconnectActions( const Interface& interface ) {
            if ( _channels.find( interface.name() ) == _channels.end() ) {
                return false;
            }
            _channels.erase( interface.name() );

            if ( _parent == interface.name() ) {
                _sendType = TokenType::ANNEXING;
                _parent = "rl0";
                _resetSent();
                _determineNextNode();
                if ( _next == "rl0" ) {
                    return _leaderActions( _id, interface.name() );
                } 
                _tokenId = _id;
                _currentPhase = 0;
                _confChanges.push_back( { ConfigAction::RESPOND, { interface.name(), Ip6Addr( "::" ), 0 } } );
                return true;
            } 
            
            if ( _leaderId == _id ) {
                _electionChangeCallback( _leaderId, ElectionStatus::CHANGED_FOLLOWERS );
                return false;
            }
            _sendType = TokenType::CHANGE;
            _confChanges.push_back( { ConfigAction::RESPOND, { interface.name(), Ip6Addr( "::" ), 0 } } );
            return true;
        }

        bool _connectActions( const Interface& interface ) {
            _channels[ interface.name() ].sent = false;
            _sendType = TokenType::CONNECT;
            _connection = interface.name();
            _confChanges.push_back( { ConfigAction::RESPOND, { interface.name(), Ip6Addr( "::" ), 0 } } );
            return true;
        }

    public:
        /** Instantiates the traversal-based election protocol. This protocol must manage every
         * module interface to start and it is not recommended to change the number of managed 
         * interfaces, unless the protocol is being completely disabled.
         * @param address The address this module will use for identification within the protocol.
         * @param callback Callback function that informs the user of changes in the election. 
         *           The Ip6Addr parameter informs of the leader's address, while ElectionStatus
         *           informs of the status of the election. Further work should be delayed until
         *           the module is out of the UNDECIDED status.
        */
        TraverseElection( const Ip6Addr& address, std::function< void ( Ip6Addr, ElectionStatus ) > callback )
        : _id( address ), _leaderId( address ), _tokenId( Ip6Addr( "::" ) ), 
          _waitingTokenId( Ip6Addr( "::" ) ), _electionChangeCallback( callback ) {
            _maxHops = -1;
            _currentPhase = -1;
            _chasedPhase = -1;
            _sendType = TokenType::INITIATE;
        }

        virtual ~TraverseElection() = default;

        virtual bool onMessage( const std::string& interfaceName,
                                rofi::hal::PBuf packetWithHeader ) override {
            auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
            Token receivedToken = as< Token >( packet.payload() );
            switch ( receivedToken.type ) {
                case TokenType::ANNEXING:
                    return _onAnnexingToken( interfaceName, receivedToken );
                case TokenType::CHASING:
                    return _onChasingToken( interfaceName, receivedToken );
                case TokenType::INITIATE:
                    return _onInitiateToken( interfaceName  );
                case TokenType::LEADER:
                    return _onLeaderToken( interfaceName, receivedToken.identity );
                case TokenType::CHANGE:
                    return _onChangeToken( interfaceName );
                case TokenType::CONNECT:
                    return _onConnectToken( interfaceName, receivedToken.identity );
                default:
                    assert( false && "Invalid message type receved\n" );
            }

            return false;
        }

        virtual bool afterMessage( const Interface& interface, 
                                   std::function< void ( PBuf&& ) > fun, void* /* args */ ) override {
            if ( _sendType == TokenType::INITIATE 
                 && _managedInterfaces.size() != rofi::hal::RoFI::getLocalRoFI().getDescriptor().connectorCount + 1 ) {
                return false;
            }

            if ( _sendType == TokenType::ANNEXING  || _sendType == TokenType::LEADER ) {
                if (  interface.name() != _next ) {
                    return false;
                }
                _channels[ interface.name() ].sent = true;
                if ( _sendType == TokenType::ANNEXING ) {
                    _maxHops++;
                }
            }

            if ( _sendType == TokenType::CHASING && interface.name() != _next ) {
                return false;
            }

            if ( _sendType == TokenType::CHANGE && interface.name() != _parent ) {
                return false;
            }

            if ( _sendType == TokenType::CONNECT && interface.name() != _connection ) {
                return false;
            }

            Token sendToken;
            sendToken.hopCount = _maxHops;
            sendToken.identity = ( _sendType == TokenType::LEADER || _sendType == TokenType::CONNECT ) ? _leaderId : _tokenId;
            sendToken.phase = _currentPhase;
            sendToken.type = _sendType;
            auto packet = PBuf::allocate( sizeof( Token ) );
            as< Token >( packet.payload() ) = sendToken;

            fun( std::move( packet ) );

            return false;
        }

        virtual bool hasConfigUpdates() const override { return!_confChanges.empty(); }

        virtual std::vector< std::pair< ConfigAction, ConfigChange > > getConfigUpdates() const {
            return _confChanges;
        }

        virtual void clearUpdates() { _confChanges.clear(); }

        virtual bool onInterfaceEvent( const Interface& interface, bool connected ) override {
            if ( _sendType != TokenType::LEADER && _sendType != TokenType::CHANGE ) {
                return false;
            }
            if ( connected ) {
                _connectActions( interface );
            } else {
                _disconnectActions( interface );
            }
            return true;
        }

        virtual bool addInterface( const Interface& interface ) {
            if ( manages( interface ) ) {
                return false;
            }

            _managedInterfaces.push_back( std::reference_wrapper( interface ) );
            if ( const_cast< Interface& >(interface).isConnected() ) {
                _channels[ interface.name() ].sent = false;
                if ( _sendType == TokenType::LEADER ) {
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

        virtual bool manages( const Interface& interface ) const override {
            return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
                return interface == i;
            } );
        }

        virtual Ip6Addr address() const override { return Ip6Addr( "ff02::ea:ea" ); }

        virtual std::string name() const override { return "traversal-election"; }

        virtual std::string info() const override {
            std::string str = Protocol::info();
            std::stringstream ss;
            ss << "; leader: " << _leaderId;
            return str  + ss.str();
        }
    };
}