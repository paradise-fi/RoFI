#include "firmware_updater.hpp"
#include <vector>
#include <optional>


void packetHandler(
        InputQueue& inputQueue, int connectorId, const Connector& connector, uint16_t contentType, PBuf data
) {
    if( contentType != FW_UPDATER_CONTENT_TYPE ) return;

    inputQueue.emplace( connectorId, std::move( data ) );
}

void registerPacketHandlers( RoFI& rofi, std::shared_ptr< InputQueue > inputQueue ) {
    // todo: acquire rofi mutex

    for( int i = 0; i < rofi.getDescriptor().connectorCount; ++i ) {
        rofi.getConnector( i ).onPacket(
            [ connectorId=i, inputQueue ] ( const Connector& c, uint16_t contentType, PBuf data ) {
                packetHandler( *inputQueue, connectorId, c, contentType, std::move(data ) );
            }
        );
    }
}

void sendMessage( std::vector<Connector>& connectors, const ConnectorId& id, const Message& m ) {
    PBuf data = serialize( m );
    connectors.at( id ).send( FW_UPDATER_CONTENT_TYPE, std::move( data ) );
}
