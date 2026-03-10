#pragma once

#include <iostream>
#include <networking/networkManager.hpp>
#include <networking/protocols/messageDistributor.hpp>
#include <set>
#include <thread>
#include <mutex>
#include <functional>
#include "electionProtocolBase.hpp"

namespace rofi::leadership {
    using namespace rofi::net;

    class LRElect : public ElectionProtocolBase {
        const unsigned int METHOD_ID = 1;
        NetworkManager& _net;
        const Ip6Addr& _myAddr;
        Ip6Addr _leader;
        std::optional< std::function<void()> > _elected_callback;
        std::optional< std::function<void()> > _failed_callback;

        std::once_flag _startedFlag;

        unsigned int _timeJoined;
        unsigned int _minTimeJoined;
        unsigned int _sequenceNumber = 0;
        unsigned int _leaderAnnouncePeriod;
        unsigned int _followerListenPeriod;

        bool _leaderContact = false;

        void _received( Ip6Addr sender_addr, uint8_t* data, unsigned int ) {
            unsigned int logTime = as< unsigned int >( data );
            if ( ( logTime < _minTimeJoined ) 
                || ( logTime == _minTimeJoined && sender_addr < _leader ) ) {
                _leader = sender_addr;
                _minTimeJoined = logTime;
            }

            if ( sender_addr == _leader ) {
                _leaderContact = true;
                _minTimeJoined = logTime;

                if ( _elected_callback.has_value() )
                {
                    _elected_callback.value()();
                }
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
            if ( _failed_callback.has_value() )
            {
                _failed_callback.value()();
            }

            _leader = _myAddr;
            _minTimeJoined = _timeJoined;
        }

        void _periodic( int initialSleep ) {
            // According to the original specification, nodes should
            // wait a bit before they start sending messages. This follows
            // that specification while also limiting the maximum time.
            // Though it may be wise to remove this, as the algorithm is still
            // better in performance than the invitation algorithm without it.
            if ( initialSleep < 3 ) {
                sleep( initialSleep );
            } else {
                sleep( 3 );
            }
            while ( true ) {
                if ( _leader == _myAddr ) {
                    if ( _elected_callback.has_value() )
                    {
                        _elected_callback.value()();
                    }
                    
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
                sleep( _leader == _myAddr ? _leaderAnnouncePeriod : _followerListenPeriod );
            }
        }

    public:
        LRElect( NetworkManager& net, 
            MessageDistributor& distributor, 
            const Ip6Addr& addr, 
            int leaderAnnouncePeriod,
            int followerListenPeriod )
        : _net( net ), _myAddr( addr ),  _leader( addr ) {
            _timeJoined = 0;
            _minTimeJoined = 0;
            _leaderAnnouncePeriod = leaderAnnouncePeriod;
            _followerListenPeriod = followerListenPeriod;
            distributor.registerMethod( METHOD_ID, 
                [ this ]( Ip6Addr address, uint8_t* data, unsigned int size ){ _received( address, data, size ); },
                [ this ](){ _increaseTimeJoined(); } );
        }

        /**
         * Start the algorithm.
         * @param id Used for an arbitrary wait time as described in the original algorithm description.
        */
        virtual void start( int id ) override {
            std::call_once( _startedFlag, [ this, id ](){
                std::thread thread{ [ this, id ]() {
                    this->_periodic( id );
                } };
                thread.detach();
            });
        }

        virtual void registerElectionFinishedCallback( std::function< void() > callBack ) override {
            _elected_callback = callBack;
        }

        virtual void registerElectionFailedCallback( std::function< void() > callBack ) override {
            _failed_callback = callBack;
        }

        virtual void unregisterCallbacks() override {
            _elected_callback.reset();
            _failed_callback.reset();
        }

        const Ip6Addr& getLeader() {
            return _leader;
        }
    };
}