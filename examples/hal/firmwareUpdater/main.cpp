#include <firmware_updater.hpp>
#include <freeRTOS.hpp>

#include <chrono>
#include <memory>
#include <numeric>

using namespace std::chrono_literals;

const FWType MY_FW_TYPE = 1;
const FWVersion MY_FW_VERSION = 1;

std::vector< Connector > getConnectors( RoFI& rofi ) {
    std::vector< Connector > connectors;
    const auto connectorCount = rofi.getDescriptor().connectorCount;

    connectors.reserve( connectorCount );

    for ( auto i = 0; i < connectorCount; ++i ) {
        connectors.emplace_back( std::move( rofi.getConnector( i ) ) );
    }

    return connectors;
}


extern "C" void app_main() {

    auto rofi = RoFI::getLocalRoFI();
    auto actualConnectors = std::make_shared< std::vector< Connector > >( getConnectors( rofi ) );

    std::vector< ConnectorId > connectorIds( actualConnectors->size() );
    std::iota( connectorIds.begin(), connectorIds.end(), 0 );

    auto runningPartition = rofi.getRunningPartition();
    const FWSize MY_FW_SIZE = runningPartition.getSize();

    auto inputQueue = std::make_shared< InputQueue >();
    auto updateProtocol = std::make_shared< UpdateProtocol >(
            MY_FW_TYPE,
            MY_FW_VERSION,
            MY_FW_SIZE,
            CHUNK_SIZE,
            rofi.getRunningPartition(),
            rofi.initUpdate(),
            connectorIds,
            [ connectors=actualConnectors ]( const ConnectorId& id, const Message& m ) {
                sendMessage( *connectors, id, m );
            }
    );

    // packet handlers are invoked in spi reading thread and add fwupdate content type packets to inputqueue
    registerPacketHandlers( rofi, inputQueue );

    FirmwareUpdaterWorker updateWorker( inputQueue, updateProtocol, 10s, 10s );
    updateWorker.start();  // start a thread that consumes messages from inputQueue and does the upgrade
//    updaterConsumer.stop();
    // todo: unregister packet handlers
    while(1) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }

}
