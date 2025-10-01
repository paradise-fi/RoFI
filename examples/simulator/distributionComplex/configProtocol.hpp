#include <networking/protocol.hpp>
#include "botState.hpp"
#include <set>

namespace rofi::net {
class ConfigurationProtocol : public Protocol
{
    BotState& _state;
    std::vector< std::reference_wrapper< const Interface > > _managedInterfaces;
    std::set< std::string > _interfaceWithCb;

    std::vector< std::pair< ConfigAction, ConfigChange > > _confChanges;

    int getConnectorId( const std::string& interfaceName )
    {
        if (interfaceName == "rd1")
        {
            return 0;
        }

        if (interfaceName == "rd2")
        {
            return 1;
        }

        if (interfaceName == "rd3")
        {
            return 2;
        }

        if (interfaceName == "rd4")
        {
            return 3;
        }

        if (interfaceName == "rd5")
        {
            return 4;
        }

        return 5;
    }
public:
    ConfigurationProtocol( BotState& state ) : _state( state ) { }

    virtual bool onMessage( const std::string& interfaceName,
                            rofi::hal::PBuf packetWithHeader ) override {
            auto packet = PBuf::own( pbuf_free_header( packetWithHeader.release(), IP6_HLEN ) );
            // Assume that a packet is just information about the moduleState
            ModuleState moduleState;
            auto payload = reinterpret_cast< const uint8_t* >( packet.payload() );
            int oppositeConnectorId = as< int >( payload );
            auto moduleStateBuffer = payload + sizeof( int );
            moduleState.deserialize( moduleStateBuffer );
            auto module = _state.modules.find( moduleState.moduleAddress );
            if ( module == _state.modules.end() )
            {
                _confChanges.push_back( { ConfigAction::RESPOND, { interfaceName, moduleState.moduleAddress, 0 } } );
            }
            _state.modules[ moduleState.moduleAddress ] = moduleState;

            if ( interfaceName != "rl0" )
            {
                int connid = getConnectorId( interfaceName );
                _state.currentModuleState().connectors[ connid ] = ConnectorStatus( connid, moduleState.moduleAddress, oppositeConnectorId );
            }

            return false;
        }

        virtual bool afterMessage( const Interface& interface, std::function< void ( PBuf&& ) > fn, void* /* args */ ) override {
            if ( interface.name() == "rl0")
            {
                return false;
            }

            auto packet = PBuf::allocate( _state.currentModuleState().size() + sizeof( int ) );
            auto payload = packet.payload();
            as< int >( payload ) = getConnectorId( interface.name() );
            auto moduleStateBuffer = payload + sizeof( int );
            _state.currentModuleState().serialize( moduleStateBuffer );
            fn( std::move( packet ) );
            return false;
        }

        virtual bool hasConfigUpdates() const override { return !_confChanges.empty(); }

        virtual std::vector< std::pair< ConfigAction, ConfigChange > > getConfigUpdates() const {
            return _confChanges;
        }

        virtual void clearUpdates() { _confChanges.clear(); }

        virtual bool addInterface( const Interface& interface ) {
            if ( manages( interface ) ) {
                return false;
            }
            _managedInterfaces.push_back( std::reference_wrapper( interface ) );
            return false;
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

        virtual bool onInterfaceEvent( const Interface&, bool ) override {
            return false;
        }

        virtual bool manages( const Interface& interface ) const override {
            return std::ranges::any_of( _managedInterfaces, [ &interface ]( const Interface& i ) {
                return interface == i;
            } );
        }

        virtual Ip6Addr address() const override { return Ip6Addr( "ff02::bb:cc" ); }

        virtual std::string name() const override { return "bot-configuration"; }

        virtual std::string info() const override {
            std::string str = Protocol::info();
            return str;
        }
    };   
};