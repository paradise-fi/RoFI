#include <networking/networkManager.hpp>
#include <lwip/udp.h>
#include <lwip++.hpp>

#include <atoms/util.hpp>

#include <set>
#include <chrono>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace rofi::leadership {

    using namespace rofi::net;
    using namespace std::chrono_literals;

    enum InvitationStatus : char {
        ELECTION,
        NORMAL,
        REORGANIZATION,
        NOT_STARTED,
    };

    enum InvitationMessage : char {
        ARE_YOU_THERE,
        ARE_YOU_THERE_RES,
        ARE_YOU_COORDINATOR,
        ARE_YOU_COORDINATOR_RES,
        INVITATION,
        READY,
        READY_RES, // Task succesfully received.
        ACCEPT,
        ACCEPT_RES, // It is necessary to confirm that a coordinator was able to register the accepted invitation. Otherwise, the coordinator could be down already.
    };

    class InvitationElection {
        struct GroupNumber {
            int coordinatorId;
            int sequenceNum;

            bool operator== ( const GroupNumber& rhs ) const {
                return coordinatorId == rhs.coordinatorId && sequenceNum == rhs.sequenceNum;
            }
        };
        
        int _id;
        InvitationStatus _nodeStatus;
        GroupNumber _groupNumber; //S(i).g
        int _groupCounter = 0; //S(i).counterF

        const Ip6Addr& _myAddr;
        Ip6Addr _coordinator;
        std::set< Ip6Addr > _up; //S(i).up
        std::set< Ip6Addr > _foundCoordinators;

        std::vector< Ip6Addr > _addresses;
        u16_t _port;
        Ip6Addr _awaited;

        udp_pcb* _pcb = nullptr;
        
        std::mutex _waitMutex;
        std::condition_variable _condVar;

        std::function< PBuf () > _calculateTask;
        std::function < void ( void * ) > _getTask;
        std::function< void () > _stopWork;

        int _timeout;
        int _period;
        std::atomic< bool > _coordinatorContacted;

        PBuf _composeMessage( InvitationMessage messageType ) {
            switch ( messageType ) {
                case InvitationMessage::ACCEPT_RES:
                case InvitationMessage::READY_RES: {
                    auto packet = PBuf::allocate( sizeof( InvitationMessage ) );
                    as< InvitationMessage >( packet.payload() ) = messageType;
                    return packet;
                }

                case InvitationMessage::INVITATION: {
                    auto packet = PBuf::allocate( sizeof( InvitationMessage ) + sizeof( GroupNumber ) + sizeof( Ip6Addr ) );
                    as< InvitationMessage >( packet.payload() ) = messageType;
                    as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) = _groupNumber;
                    as< Ip6Addr >( packet.payload() + sizeof( InvitationMessage ) + sizeof( GroupNumber ) ) = _coordinator;
                    return packet;
                }
                case InvitationMessage::ARE_YOU_THERE: {
                    auto packet = PBuf::allocate( sizeof( InvitationMessage ) + sizeof( GroupNumber ) + sizeof( int ) );
                    as< InvitationMessage >( packet.payload() ) = messageType;
                    as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) = _groupNumber;
                    as< int >( packet.payload() + sizeof( InvitationMessage ) + sizeof( GroupNumber ) ) = _id;
                    return packet;
                }
                case InvitationMessage::ARE_YOU_COORDINATOR:
                case InvitationMessage::ACCEPT: {
                    auto packet = PBuf::allocate( sizeof( InvitationMessage ) + sizeof( GroupNumber ) + sizeof( int ) );
                    as< InvitationMessage >( packet.payload() ) = messageType;
                    as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) = _groupNumber;
                    return packet;
                }

                default: {
                    assert( false && "Incorrect message type received at _composeMessage\n");
                }
            }
        }

        PBuf _composeResponse( InvitationMessage messageType, bool answer ) {
            int size = sizeof( InvitationMessage ) + sizeof( bool );
            auto packet = PBuf::allocate( size );
            as< InvitationMessage >( packet.payload() ) = messageType;
            as< bool >( packet.payload() + sizeof( InvitationMessage ) ) = answer;
            return packet;
        }

        PBuf _composeReadyMessage( ) {
            PBuf task = _calculateTask();
            PBuf packet = PBuf::allocate( sizeof( InvitationMessage ) + sizeof( GroupNumber ) + task.size() * sizeof( uint8_t ) );
            as< InvitationMessage >( packet.payload() ) = InvitationMessage::READY;
            as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) = _groupNumber;
            auto* taskData = task.payload();
            auto* packetData = packet.payload() + sizeof( InvitationMessage ) + sizeof( GroupNumber );
            
            for ( int i = 0; i <= task.size(); i++ ) {
                *packetData = *taskData;
                taskData++;
                packetData++;
            }

            return packet;
        }

        bool _sendMessage( const Ip6Addr& ip, PBuf packet ){
            assert( _pcb && "PCB is null.\n" );
            Ip6Addr addr = ip;
            addr.zone = 0;
            err_t res = udp_sendto( _pcb, packet.release(), &addr, _port );
            assert( res != ERR_MEM && "Out of memory.\n" );
            return res == ERR_OK;
        }

        void _onAcceptMessage( const Ip6Addr addr, PBuf packet ) {
            GroupNumber group = as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) );
            if ( _nodeStatus == InvitationStatus::ELECTION 
                 && group == _groupNumber 
                 && _coordinator == _myAddr ) {
                _up.emplace( addr );
                _awaited = _myAddr;
                _sendMessage( addr, _composeMessage( InvitationMessage::ACCEPT_RES ) );
            }
        }

        void _onAcceptResponse( const Ip6Addr& addr ) {
            if ( _awaited != addr ) {
                _recovery();
                return;
            }
            _awaited = _myAddr;
            _nodeStatus = InvitationStatus::REORGANIZATION;
        }

        void _onAreYouThere( const Ip6Addr& addr, GroupNumber groupNum ) {
            auto res = std::find_if( _up.begin(), _up.end(), [ addr ]( const Ip6Addr& address ) { return address == addr; } );
            _sendMessage( addr, _composeResponse( InvitationMessage::ARE_YOU_THERE_RES, 
                                                  groupNum == _groupNumber 
                                                  && _myAddr == _coordinator 
                                                  && res != _up.end() ) );
        }

        void _onAreYouCoordinator( const Ip6Addr& addr, GroupNumber groupNum ) {
            if ( addr == _coordinator && _groupNumber == groupNum ) {
                _coordinatorContacted = true;
            }
            _sendMessage( addr, _composeResponse( InvitationMessage::ARE_YOU_COORDINATOR_RES, 
                                                          _nodeStatus == InvitationStatus::NORMAL && _myAddr == _coordinator ) );
        }

        void _onAreYouThereRes( const Ip6Addr& addr, PBuf packet ) {
            if ( addr != _awaited ) {
                return;
            }
            bool response = as< bool >( packet.payload() + sizeof( InvitationMessage ) );
            if ( response ) {
                _awaited = _myAddr;
            }
        }

        void _onAreYouCoordinatorRes( const Ip6Addr& addr, PBuf packet ) {
            if ( addr != _awaited ) {
                return;
            }
            bool response = as< bool >( packet.payload() + sizeof( InvitationMessage ) );
            if ( response ) {
                _foundCoordinators.emplace( addr );
            }
            _awaited = _myAddr;
        }

        void _onInvitation( GroupNumber group, const Ip6Addr& newCoordinator ) {
            if ( _nodeStatus != InvitationStatus::NORMAL ) {
                return;
            }
            _nodeStatus = InvitationStatus::ELECTION;
            _stopWork();
            bool wasCoordinator = _coordinator == _myAddr;
            std::set< Ip6Addr > tempUp = _up;
            _up.clear();

            _coordinator = newCoordinator;
            _groupNumber = group;

            if ( wasCoordinator ) {
                for ( const Ip6Addr& address : tempUp ) {
                    _sendMessage( address, _composeMessage( InvitationMessage::INVITATION ) );
                }
            }

            if ( !_sendMessage( newCoordinator, _composeMessage( InvitationMessage::ACCEPT ) ) ) {
                _recovery();
            }
            
            _awaited = newCoordinator;
            return;
        }

        void _onReadyMessage( const Ip6Addr& addr, PBuf packet ) {
            GroupNumber group = as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) );
            if ( _nodeStatus == InvitationStatus::REORGANIZATION && _groupNumber == group ) {
                _getTask( static_cast< void* > ( packet.payload() + sizeof( InvitationMessage ) + sizeof( GroupNumber ) ) );
                _nodeStatus = InvitationStatus::NORMAL;
                _sendMessage( addr, _composeMessage( InvitationMessage::READY_RES ) );
            }
        }

        void _recovery() {
            _nodeStatus = InvitationStatus::ELECTION;
            _stopWork();
            _groupCounter++;
            _groupNumber.sequenceNum = _groupCounter;
            _groupNumber.coordinatorId = _id;
            _coordinator = _myAddr;
            _up.clear();
            _up.emplace( _myAddr );
            _nodeStatus = InvitationStatus::REORGANIZATION;
            PBuf task = _calculateTask();
            _getTask( static_cast< void* > ( task.payload() ) );
            _nodeStatus = InvitationStatus::NORMAL;
        }

        void _mergeGroups() {
            _nodeStatus = InvitationStatus::ELECTION;
            _stopWork();
            _groupCounter++;
            _groupNumber.sequenceNum = _groupCounter;
            _groupNumber.coordinatorId = _id;
            _coordinator = _myAddr;

            std::set< Ip6Addr > tempSet = _up; // Use references? Probably not, since we need to copy the set as it gets erased later.
            _up.clear();
            _up.emplace( _myAddr );
            for ( const Ip6Addr& coordinator : _foundCoordinators ) {
                _sendMessage( coordinator, _composeMessage( InvitationMessage::INVITATION ) );
            }
            for ( const Ip6Addr& address : tempSet ) {
                _sendMessage( address, _composeMessage( InvitationMessage::INVITATION ) );
            }

            // Wait ... some time.
            sleep( _timeout * 3 );
            _nodeStatus = InvitationStatus::REORGANIZATION;
            for ( const Ip6Addr& address : _up ) {
                if ( address == _myAddr ) {
                    continue;
                }
                _awaited = address;
                _sendMessage( address, _composeReadyMessage() );
                std::unique_lock< std::mutex > lock( _waitMutex );
                if ( !_condVar.wait_for( lock, std::chrono::seconds( _timeout ), [ this ] { return _awaited == _myAddr; } ) ) {
                    _awaited = _myAddr;
                    _recovery();
                    return;
                }
                lock.unlock();
            }
            PBuf task = _calculateTask();
            _getTask( static_cast< void* > ( task.payload() ) );
            _nodeStatus = InvitationStatus::NORMAL;
        }

        void _checkForGroups() {
            _foundCoordinators.clear();
            if ( _nodeStatus == InvitationStatus::NORMAL && _coordinator == _myAddr ) {
                for ( auto address : _addresses ) {
                    if ( address == _myAddr ) {
                        continue;
                    }
                    _awaited = address;
                    if ( !_sendMessage( address, _composeMessage( InvitationMessage::ARE_YOU_COORDINATOR ) ) ) {
                        continue;
                    }
                    std::unique_lock< std::mutex > lock( _waitMutex );
                    if ( !_condVar.wait_for( lock, std::chrono::seconds( _timeout ), [ this ] { return _awaited == _myAddr; } ) ) {
                        continue;
                    }
                    lock.unlock();
                }
                _awaited = _myAddr; // Since we never send a message to ourselves, we can consider _myAddr as undefined for _awaited.
                if ( _foundCoordinators.empty() ) {
                    return;
                }

                Ip6Addr highestPrio = *_foundCoordinators.rbegin();
                if ( highestPrio > _myAddr ) {
                    sleep( _timeout );
                    return; // We yield to the coordinator of larger priority, since they are likely going to invite us.
                }
                _mergeGroups();
            }
        }

        void _checkCoordinator() {
            _awaited = _coordinator;
            if ( !_sendMessage( _coordinator , _composeMessage( InvitationMessage::ARE_YOU_THERE ) ) ) {
                _awaited = _myAddr; 
                _recovery();
                return;
            }

            std::unique_lock< std::mutex > lock( _waitMutex );
            if ( !_condVar.wait_for( lock, std::chrono::seconds( _timeout ), [ this ]{ return _awaited == _myAddr; } ) ) {
                _awaited = _myAddr; 
                _recovery();
            }
            lock.unlock();
            return;
        }

        void _periodicCheck() {
            while ( true ) {
                if ( _nodeStatus == InvitationStatus::NORMAL && _coordinator == _myAddr ) {
                    _checkForGroups();
                } 
                if ( _coordinator != _myAddr ) {
                    if ( _coordinatorContacted ) {
                        _coordinatorContacted = false;
                    } else {
                        _checkCoordinator();
                    }
                }
                sleep( _period );
            }
        }
    public:
        /**
         * The Invitation Election class constructor.
         * IMPORTANT: Invitation Election requires an underlying routing protocol
         * to be active on all modules!
         * @param id The RoFI module's ID, unique to the configuration.
         * @param myAddr the address used for identification by the module. Should be unique and contained in addresses.
         * @param port The port for the underlying networking service used for the election.
         * @param addresses a vector of all possible node addresses within the system, make sure there is only one address per one module.
         * @param calculateTask a function used when the election requires a task for distribution.
         * @param getTask a function used when the election receives a task and is passing it onto the rest of the code.
         * @param stopWork a function that is invoked when the node status becomes unstable, an election is ongoing, and work needs to be stopped until new tasks are distributed.
         * @param timeout allows the user to specify how long an operation waits for a response until timeout is decalred. The default is 1.
         * @param period allows the user to specify how long the module waits between checks for the existence of a leader or other groups. The default is 3.
        */
        InvitationElection( int id, Ip6Addr& myAddr, u16_t port,  
                            std::vector< Ip6Addr > addresses, 
                            std::function< PBuf () > calculateTask, 
                            std::function< void ( void* ) > getTask,
                            std::function< void () > stopWork,
                            unsigned int timeout, unsigned int period ) 
        : _myAddr( myAddr), _coordinator( myAddr ), _awaited( myAddr ) {
            assert( id >= 0 && "ID must be non-negative!\n");
            _id = id;
            _nodeStatus = InvitationStatus::NOT_STARTED;
            _groupNumber.coordinatorId = _id;
            _groupNumber.sequenceNum = 0;
            _timeout = timeout;
            _period = period;
            _port = port;
            _calculateTask = calculateTask;
            _getTask = getTask;
            _stopWork = stopWork;
            _addresses = addresses;
        }

        InvitationElection( int id, Ip6Addr& myAddr, u16_t port,
                            std::vector< Ip6Addr > addresses,
                            std::function< PBuf () > calculateTask, 
                            std::function< void ( void* ) > getTask, 
                            std::function< void () > stopWork )
        : InvitationElection( id, myAddr, port, addresses, calculateTask, getTask, stopWork, 1, 3 ){};

        /** Sets up the necessary network environment for the algorithm.
         * Remember to call setUp() before start().
         * @return true if all went well, false is something went wrong.
        */
        bool setUp() {
            if ( _nodeStatus != InvitationStatus::NOT_STARTED ) {
                return false;
            }

            _pcb = udp_new();
            if ( _pcb == nullptr ) {
                return false;
            }
            
            err_t bind = udp_bind( _pcb, IP6_ADDR_ANY, _port );
            if ( bind != ERR_OK ) {
                return false;
            }
            
            udp_recv( _pcb, 
                        [ ] ( void* invCls, struct udp_pcb*, struct pbuf* p, const ip6_addr_t* addr, u16_t ) {
                            if ( !p || !addr ) {
                                return;
                            }
    
                            auto packet = PBuf::own( p );
                            as< InvitationElection >( invCls ).onMessage( Ip6Addr( *addr ), packet );
                    }, this ); // Here, we only want to pass the onMessage function, not the whole class.
            return true;
        }

        /** Starts the algorithm execution for the given module.
         * Remember to call setUp() before calling start().
        */
        void start() {
            if ( _nodeStatus != InvitationStatus::NOT_STARTED ) {
                return;
            }

            _recovery();
            std::thread thread{ [ this ]() {
                this->_periodicCheck();
            } };
            thread.detach();
        }

        /**
         * Returns the identity of the node's assumed leader, as well
         * as an indicator as to whether the leader is up to date ( second == true ), 
         * or whether the leader is currently being re-elected ( second == false ).
        */
        std::pair< const Ip6Addr&, bool > getLeader() {
            return std::pair< Ip6Addr&, bool >( _coordinator, _nodeStatus == InvitationStatus::NORMAL );
        }

        /**
         * Retrieves the modules this current module believes are available.
        */
        const std::set< Ip6Addr > getUp() {
            return _up;
        }

        void onMessage( const Ip6Addr addr, PBuf packet ) {
            if (  _nodeStatus == InvitationStatus::NOT_STARTED ) {
                return;
            }

            InvitationMessage messageType = as< InvitationMessage >( packet.payload() );
            switch ( messageType ) {
                case InvitationMessage::ACCEPT:
                    _onAcceptMessage( addr, packet );
                    return;
                case InvitationMessage::ACCEPT_RES:
                    _onAcceptResponse( addr );
                    return;
                case InvitationMessage::ARE_YOU_COORDINATOR:
                    _onAreYouCoordinator( addr, as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) );
                    return;
                case InvitationMessage::ARE_YOU_COORDINATOR_RES:
                    _onAreYouCoordinatorRes( addr, packet );
                    return;
                case InvitationMessage::ARE_YOU_THERE:
                    _onAreYouThere( addr, as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) ) );
                    return;
                case InvitationMessage::ARE_YOU_THERE_RES:
                    _onAreYouThereRes( addr, packet );
                    return;
                case InvitationMessage::INVITATION: {
                    GroupNumber grpNum = as< GroupNumber >( packet.payload() + sizeof( InvitationMessage ) );
                    Ip6Addr newCoord = as< Ip6Addr >( packet.payload() + sizeof( InvitationMessage ) + sizeof( GroupNumber ) );
                    _onInvitation( grpNum, newCoord );
                    return;
                }
                case InvitationMessage::READY: {
                    _onReadyMessage( addr, packet );
                    return;
                }
                case InvitationMessage::READY_RES: {
                    if ( _awaited != addr ) {
                        return;
                    }
                    _awaited = _myAddr;
                }
            }
            return;
        }
    };
}