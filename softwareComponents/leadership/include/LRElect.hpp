#pragma once

#include <networking/networkManager.hpp>
#include <LRHelper.hpp>
#include <set>
#include <thread>
#include <functional>

namespace rofi::leadership {
    using namespace rofi::net;

    class LRElect {
        NetworkManager& _net;
        const Ip6Addr& _myAddr;
        Ip6Addr _leader;

        std::once_flag _startedFlag;

        unsigned int _timeJoined;
        unsigned int _minTimeJoined;
        unsigned int _sequenceNumber = 0;
        unsigned int _period;

        bool _leaderContact = false;

        /** Called by the helper protocol to pass on a message. */
        void _received( Ip6Addr addr, unsigned int logTime ) {
            if ( ( logTime < _minTimeJoined ) 
                || ( logTime == _minTimeJoined && addr < _leader ) ) {
                _leader = addr;
                _minTimeJoined = logTime;
            }

            if ( addr == _leader ) {
                _leaderContact = true;
                _minTimeJoined = logTime;
            }
        }

        /** Called by the helper protocol when a connector event occurs. */
        void _increaseTimeJoined() {
            _timeJoined++;
            if ( _leader == _myAddr) {
                _minTimeJoined = _timeJoined;
            }
        }

        void _leaderFailure() {
            _leader = _myAddr;
            _minTimeJoined = _timeJoined;
        }

        void _periodic( int id ) {
            // According to the original specification, nodes should
            // wait a bit before they start sending messages. This follows
            // that specification while also limiting the maximum time.
            // Though it may be wise to remove this, as the algorithm is still
            // better in performance than the invitation algorithm without it.
            if ( id < 3 ) {
                sleep( id );
            } else {
                sleep( 3 );
            }
            while ( true ) {
                if ( _leader == _myAddr ) {
                    for ( const Interface& interface : _net.interfaces() ) {
                        if ( interface.name() == "rl0" ) {
                            continue;
                        }
                        PBuf packet = PBuf::allocate( Ip6Addr::size() + 2 * sizeof( unsigned int ) );
                        as< Ip6Addr >( packet.payload() ) = _myAddr;
                        as< unsigned int >( packet.payload() + Ip6Addr::size() ) = _timeJoined;
                        as< unsigned int >( packet.payload() + Ip6Addr::size() + sizeof( unsigned int ) ) = _sequenceNumber;
                        if ( ! const_cast< Interface& >( interface ).sendProtocol( _net.getProtocol( "lr-helper" )->address(), std::move( packet ) ) ) {
                            assert( false && "failed to send a message to the helper protocol. Something went wrong." );
                        }
                    }
                    _sequenceNumber++;
                } else {
                    if ( !_leaderContact ) {
                        _leaderFailure();
                    }

                    _leaderContact = false;
                }
                sleep( _period );
            }
        }

    public:
        /** 
         * The LRElect class constructor.
         * @param net A reference to a RoFI network manager class instance.
         * @param addr A reference to the address that this module will use as its identity for the election. Must be unique.
         * @param period The period of seconds between election mechanisms. Default = 3.
        */
        LRElect( NetworkManager& net, const Ip6Addr& addr, unsigned int period ) : _net( net ), _myAddr( addr ),  _leader( addr ) {
            _timeJoined = 0;
            _minTimeJoined = 0;
            _period = period;
            std::function< void ( const Ip6Addr, unsigned int ) > fun = [ this ]( const Ip6Addr address, unsigned int logTime ){ _received( address, logTime); };
            Protocol* prot = net.addProtocol( LRHelper( addr, fun, [ this ](){ _increaseTimeJoined(); } ) );
            net.setProtocol( *prot );
        }

        LRElect( NetworkManager& net, const Ip6Addr& addr ) : LRElect( net, addr, 3 ){}

        /**
         * Start the algorithm.
         * @param id Used for an arbitrary wait time as described in the original algorithm description.
        */
        void start( int id ) {
            std::call_once( _startedFlag, [ this, id ](){
                std::thread thread{ [ this, id ]() {
                    this->_periodic( id );
                } };
                thread.detach();
            });
        }

        const Ip6Addr& getLeader() {
            return _leader;
        }
    };
}