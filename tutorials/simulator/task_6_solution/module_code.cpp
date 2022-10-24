#include <future>
#include <iostream>

#include <atoms/units.hpp>

#include <rofi_hal.hpp>


namespace hal = rofi::hal;
using namespace std::chrono_literals;

void blockingWait( std::chrono::milliseconds delayMs )
{
    auto waitEndPromise = std::promise< void >();

    hal::RoFI::wait( static_cast< int >( delayMs.count() ), [ & ] { waitEndPromise.set_value(); } );

    waitEndPromise.get_future().get();
}

hal::PBuf createPacket( const std::string & message )
{
    auto packet = hal::PBuf::allocate( int( message.size() ) );

    auto msgPtr = message.data();
    for ( auto it = packet.chunksBegin(); it != packet.chunksEnd(); ++it ) {
        auto size = std::min< std::ptrdiff_t >( it->size(),
                                                message.size() - ( msgPtr - message.data() ) );
        if ( size <= 0 ) {
            break;
        }
        std::copy_n( msgPtr, size, it->mem() );
    }

    return packet;
}


int main()
{
    auto localRoFI = hal::RoFI::getLocalRoFI();
    std::cout << "Hello world from module " << localRoFI.getId() << "!\n";

    auto connector = localRoFI.getConnector( 0 );

    connector.onPacket( []( hal::Connector, uint16_t, hal::PBuf packet ) {
        std::cout << "Got message: " << packet.asString() << "\n";
    } );

    for ( int i = 0; true; i++ ) {
        blockingWait( 1s );
        connector.send( 1, createPacket( "Message no. " + std::to_string( i ) ) );
    }
}
