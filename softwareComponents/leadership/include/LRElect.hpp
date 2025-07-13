#pragma once

#include <iostream>
#include <networking/networkManager.hpp>
#include <LRHelper.hpp>
#include <networking/protocols/messageDistributor.hpp>
#include <set>
#include <thread>
#include <mutex>
#include <functional>

namespace rofi::leadership {
    using namespace rofi::net;

    class LRElect {
        const unsigned int METHOD_ID = 1;
        NetworkManager& _net;
        const Ip6Addr& _myAddr;
        Ip6Addr _leader;
        std::function<void()> _elected_callback;
        std::function<void()> _failed_callback;

        std::once_flag _startedFlag;

        unsigned int _timeJoined;
        unsigned int _minTimeJoined;
        unsigned int _sequenceNumber = 0;
        unsigned int _period;

        bool _leaderContact = false;

        void _received( Ip6Addr sender_addr, uint8_t* data, unsigned int ) {
            unsigned int logTime = as< unsigned int >( data );
            if ( ( logTime < _minTimeJoined ) 
                || ( logTime == _minTimeJoined && sender_addr < _leader ) ) {
                _leader = sender_addr;
                _minTimeJoined = logTime;
            }

            if ( sender_addr == _leader ) {
                _elected_callback();
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
            _failed_callback();
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
                    _elected_callback();
                    
                    Protocol* proto = _net.getProtocol( "message-distributor" );
                    if ( proto == nullptr )
                    {
                        std::cout << "Protocol message-distributor not found!" << std::endl;
                        return;
                    }

                    auto distributor = reinterpret_cast< MessageDistributor* >( proto );

                    unsigned int data = _timeJoined;

                    distributor->sendMessage( _myAddr, METHOD_ID, reinterpret_cast< uint8_t* >( &data ), sizeof( unsigned int ) );
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
        LRElect( NetworkManager& net, 
            MessageDistributor* distributor, 
            const Ip6Addr& addr, 
            unsigned int, 
            std::function<void()> elected_callback,
            std::function<void()> failed_callback)
        : _net( net ), _myAddr( addr ),  _leader( addr ),
                _elected_callback( elected_callback ),
                _failed_callback( failed_callback ) {
            _timeJoined = 0;
            _minTimeJoined = 0;
            _period = 1;
            distributor->registerMethod( METHOD_ID, 
                [ this ]( Ip6Addr address, uint8_t* data, unsigned int size ){ _received( address, data, size ); },
                [ this ](){ _increaseTimeJoined(); } );
        }

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