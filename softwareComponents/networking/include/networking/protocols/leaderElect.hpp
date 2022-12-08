#pragma once

#include "lwip++.hpp"
#include <networking/protocol.hpp>
#include <networking/interface.hpp>
#include <networking/routingTable.hpp>

#include <atoms/util.hpp>

#include <vector>
#include <functional>
#include <map>

namespace rofi::net {

/**
 * \brief Proof-of-concept of a leader election protocol using IP layer only.
*/
class LeaderElect : public Protocol {
    Ip6Addr _leaderAddress;
    uint8_t _mask;
    int _leaderId;
    int _id;

    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::map< Interface::Name, bool > _converged;
    std::vector< std::pair< ConfigAction, ConfigChange > > _confChanges;

public:
    LeaderElect( int id, const Ip6Addr& leaderAddr, uint8_t mask )
    : _leaderAddress( leaderAddr ), _mask( mask ), _id( id ) {
        _leaderId = _id;
    }

    virtual ~LeaderElect() = default;

    virtual bool onMessage( const std::string& interfaceName, rofi::hal::PBuf packetWithHeader ) {
        bool result = false;

        auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
        int otherId = as< int >( packet.payload() );

        _converged[ interfaceName ] = otherId == _leaderId;
        if ( !_converged[ interfaceName ] ) {
            if ( otherId < _leaderId ) {
                if ( _leaderId == _id ) {
                    _confChanges.push_back( { ConfigAction::REMOVE_IP, { "rl0", _leaderAddress, _mask } } );
                    result = true;
                }

                _leaderId = otherId;
                for ( const auto& [ k, _ ] : _converged ) {
                    _converged[ k ] = false;
                }
            } else if ( _id == _leaderId ) {
                _confChanges.push_back( { ConfigAction::ADD_IP, { "rl0", _leaderAddress, _mask } } );
                result = true;
            } else {
                // ToDo: Think about doing this optional.
                _confChanges.push_back( { ConfigAction::RESPOND, { "", Ip6Addr( "::" ), 0 } } );
                result = true;
            }
        } if ( _id == _leaderId ) {
            _confChanges.push_back( { ConfigAction::ADD_IP, { "rl0", _leaderAddress, _mask } } );
            result = true;
        }

        return result;
    }

    virtual bool afterMessage( const Interface& i, std::function< void ( PBuf&& ) > f, void* /* args */ ) {
        if ( _converged[ i.name() ] )
            return false;

        auto packet = PBuf::allocate( sizeof( int ) );
        as< int >( packet.payload() ) = _leaderId;
        f( std::move( packet ) );

        return false;
    }

    virtual bool hasConfigUpdates() const override { return !_confChanges.empty(); }

    virtual std::vector< std::pair< ConfigAction, ConfigChange > > getConfigUpdates() const {
        return _confChanges;
    }

    virtual void clearUpdates() { _confChanges.clear(); }

    virtual bool addInterface( const Interface& interface ) {
        if ( manages( interface ) )
            return false;

        _managedInterfaces.push_back( std::reference_wrapper( interface ) );
        _converged[ interface.name() ] = false;
        return true;
    }

    virtual bool removeInterface( const Interface& interface ) {
        auto it = std::find_if( _managedInterfaces.begin(), _managedInterfaces.end()
                              , [ &interface ]( const auto& i ) { return interface == i; } );
        if ( it == _managedInterfaces.end() )
            return false;

        std::swap( *it, _managedInterfaces.back() );
        _managedInterfaces.pop_back();
        return true;
    }

    virtual bool manages( const Interface& interface ) const override {
        return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
            return interface == i;
        } );
    }

    virtual Ip6Addr address() const override { return "ff02::aa:ff"; }

    virtual std::string name() const override { return "leader-elect"; }

    virtual std::string info() const override {
        std::string str = Protocol::info();
        std::stringstream ss;
        ss << "; leader id: " << _leaderId << " leader address: " << _leaderAddress;
        return str  + ss.str();
    }
};

} // namespace rofi::net

