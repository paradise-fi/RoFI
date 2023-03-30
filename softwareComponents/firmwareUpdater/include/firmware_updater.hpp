#pragma once

#include "bounded_queue.hpp"
#include "messages.hpp"
#include "rofi_hal.hpp"
#include "serialization.hpp"
#include "update_protocol.hpp"
#include "rs_store.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <memory>
#include <thread>
#include <tuple>
#include <utility>

using namespace rofi::hal;
using namespace rofi::updater::messages;
using namespace std::chrono_literals;

using InputQueueEntry = std::pair< int, PBuf >;
const int INPUT_QUEUE_CAPACITY = 8;
using InputQueue = BoundedQueue< InputQueueEntry, INPUT_QUEUE_CAPACITY >;

const uint16_t FW_UPDATER_CONTENT_TYPE = 3;

void registerPacketHandlers( RoFI& rofi, std::shared_ptr< InputQueue > inputQueue );

void sendMessage( std::vector< Connector >& connectors, const ConnectorId& id, const Message& m );

class FirmwareUpdateWorker {
public:
    FirmwareUpdateWorker(std::shared_ptr< InputQueue > inputQueue, std::shared_ptr< UpdateProtocol > updateProtocol,
                         const std::chrono::steady_clock::duration& announcePeriod,
                         const std::chrono::steady_clock::duration& progressCheckPeriod )
            : _inputQueue( std::move( inputQueue ) ), _updateProtocol( std::move( updateProtocol ) ),
            _nextPeriodicAnnounce( std::chrono::steady_clock::now() ),
            _nextProgressCheck( std::chrono::steady_clock::now() ), _announcePeriod( announcePeriod ),
            _progressCheckPeriod( progressCheckPeriod ) {};

    FirmwareUpdateWorker(const FirmwareUpdateWorker& ) = delete;
    FirmwareUpdateWorker& operator=(const FirmwareUpdateWorker& ) = delete;

    void start() {
        if ( _workerThread.joinable() ) throw std::runtime_error( "worker thread is already running" );

        _workerThread = std::jthread( [&]( std::stop_token stopToken ) { _run( std::move( stopToken ) ); } );
    }

    void stop() {
        if ( ! _workerThread.joinable() ) return;
        _workerThread.request_stop();
        _workerThread.join();
    }

    ~FirmwareUpdateWorker() {
        stop();
    }

private:
    void _run( std::stop_token stopToken ) {
        std::chrono::steady_clock::duration timeout = 0s;

        while ( true ) {
            Message message;
            {
                auto entry = _inputQueue->dequeue( stopToken, timeout );
                if ( stopToken.stop_requested() ) return;
                timeout = _handleWakeUp();
                if ( ! entry ) continue;

                auto &[ connectorId, packet ] = *entry;
                message = deserialize( packet );
                Proto& proto = _proto( message );
                proto.connectorId = connectorId;
            }

            _updateProtocol->onMessage( message );
        }
    }

    auto _handleWakeUp() -> std::chrono::steady_clock::duration {
        const auto now = std::chrono::steady_clock::now();
        if ( _nextPeriodicAnnounce <= now ) {
            _updateProtocol->announceRunningFirmware();
            _nextPeriodicAnnounce = now + _announcePeriod;
        }

        if ( now > _nextProgressCheck && _updateProtocol->updating() ) {
            const auto lastProgressAt = _updateProtocol->lastProgressAt().value();
            if ( lastProgressAt + _progressCheckPeriod <= now ) {
                _updateProtocol->requestNextMissingChunk();
                _nextProgressCheck = now + _progressCheckPeriod;
            }
        } else {
            _nextProgressCheck = now + _progressCheckPeriod;
        }

        return std::min( _nextPeriodicAnnounce - now, _nextProgressCheck - now );
    }

    std::jthread _workerThread;
    std::shared_ptr< InputQueue > _inputQueue;
    std::shared_ptr< UpdateProtocol > _updateProtocol;

    const std::chrono::steady_clock::duration _announcePeriod;
    const std::chrono::steady_clock::duration _progressCheckPeriod;

    std::chrono::steady_clock::time_point _nextPeriodicAnnounce;
    std::chrono::steady_clock::time_point _nextProgressCheck;
};
