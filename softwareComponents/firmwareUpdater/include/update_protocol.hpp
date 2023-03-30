#pragma once

#include "messages.hpp"
#include "rs_store.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <rofi_hal.hpp>
#include <span>
#include <utility>

namespace rofi::updater {
using namespace rofi::updater::messages;

using Clock = std::chrono::steady_clock;

const ConnectorId MYSELF_CONNECTOR_ID = -1;

struct Firmware {
    FWVersion version;
    FWType type;
    FWSize size;
};


struct OngoingUpdate {
    FWType fwType;
    FWVersion fwVersion;
    Proto proto;
    Firmware candidateFirmware;  // todo: maybe abstract it to UpdatePartition?? -- we need something writable
    Clock::time_point lastProgress;
    std::vector< bool > _presentChunks;
    hal::UpdatePartition _updatePartition;

    [[nodiscard]] bool isChunkPresent( const ChunkId& chunkId ) const {
        assert( chunkId < _presentChunks.size() );
        return _presentChunks[ chunkId ];
    }

    void markProgress() {
        lastProgress = Clock::now();
    }

    [[nodiscard]] bool isComplete() const {
        return std::all_of(_presentChunks.begin(), _presentChunks.end(), []( bool x ){ return x; } );
    }

    void commit() {
        assert( isComplete() );
        _updatePartition.commit();
    }

    void writeChunk( const ChunkId& chunkId, const std::span< const unsigned char >& data ) {
        _updatePartition.write( chunkId * proto.chunkSize, data );
        _presentChunks[ chunkId ] = true;
    }

    std::optional< ChunkId > nextAvailableChunkId( const ChunkDescriptor& dsc, const Proto& proto ) {
        assert( this->proto.chunkSize == proto.chunkSize );
        if ( dsc.chunkId >= _presentChunks.size() ) return {};
        auto it = std::find( _presentChunks.begin() + dsc.chunkId, _presentChunks.end(), true );
        if ( it == _presentChunks.end() ) return {};
        return it - _presentChunks.begin();  // todo: does the first chunk has id of 0??
    }

    std::optional< ChunkId > firstMissingChunkId() {
        auto it = std::find( _presentChunks.begin(), _presentChunks.end(), false );
        if ( it == _presentChunks.end() ) return {};
        return { it - _presentChunks.begin() };
    }

};


std::size_t _actualChunkSize( std::size_t totalSize, ChunkId chunkId, std::size_t chunkSize );

ChunkId _sizeToChunks( std::size_t size, std::size_t chunkSize );

bool _isValidChunkId( std::size_t totalSize, const ChunkId& chunkId, std::size_t chunkSize );


class UpdateProtocol {
public:
    using send_message_t = std::function< void( const ConnectorId&, const Message& ) >;
    UpdateProtocol( const FWType fwType, const FWVersion fwVersion, const FWSize fwSize, const ChunkSize chunkSize, hal::Partition runningPartition,
                    hal::UpdatePartition updatePartition, std::vector< ConnectorId > connectors, send_message_t sendMessage )
            : _fwType( fwType ), _chunkSize( chunkSize ), _runningPartition( std::move( runningPartition ) ),
            _updatePartition( std::move( updatePartition ) ), _connectors( std::move( connectors ) ),
            _sendMessage( std::move( sendMessage ) ), _runningFirmware( {fwVersion, fwType, fwSize} ){}

    void onMessage( const Message& m ) {
        if( ! _validMessage( m ) ) return;

        std::visit(
            [&]( const auto& m ) { return _onMessage( m ); },
            m
        );
    }

    void _onMessage( const AnnounceMessage& m ) {
        if ( ! _matchingFWTypeMessage( m ) ) {
            _processIncompatibleMessage( m );
            return;
        }
        if ( m.dsc.fwVersion <= _runningFirmware.version ) return;

        if ( ! _ongoingUpdate )
            _initUpdate(m.proto, m.dsc );

        if ( m.dsc.fwVersion != _ongoingUpdate->fwVersion ) return;

        if ( _ongoingUpdate->isChunkPresent( m.dsc.chunkId ) ) return;

        _requestChunkFromDevice( m.proto.connectorId, m.dsc, m.proto );
        _ongoingUpdate->markProgress();
    }

    void _onMessage( const RequestMessage& m ) {
        if ( ! _matchingFWTypeMessage( m ) ) {
            _processIncompatibleMessage( m );
            return;
        }

        if ( m.dsc.fwVersion == _runningFirmware.version ) {
            if ( !_isValidChunkId( _runningFirmware.size, m.dsc.chunkId, m.proto.chunkSize ) ) return;

            _sendData( m.dsc, m.proto.connectorId, _runningPartition, m.proto );

            auto nextChunk = _nextAvailableChunkId(m.dsc, _runningFirmware.size, m.proto.chunkSize );
            if ( nextChunk )
                _announceChunkToDevice( { m.dsc.fwType, m.dsc.fwVersion, *nextChunk }, m.proto.connectorId, m.proto );
            return;
        }

        if ( ! _ongoingUpdate ) return;
        if ( m.dsc.fwVersion != _ongoingUpdate->fwVersion ) return;
        if ( ! _ongoingUpdate->isChunkPresent( m.dsc.chunkId ) ) {
            if ( ! _isValidChunkId( _ongoingUpdate->candidateFirmware.size, m.dsc.chunkId, m.proto.chunkSize ) ) return;

            _requestChunkForDevice( m.proto.connectorId, m.dsc, m.proto );
            _requestChunkForDevice( MYSELF_CONNECTOR_ID, m.dsc, m.proto );
            return;
        }

        _sendData( m.dsc, m.proto.connectorId, _ongoingUpdate->_updatePartition, m.proto );
        auto nextChunk = _ongoingUpdate->nextAvailableChunkId( m.dsc, m.proto );
        if ( nextChunk )
            _announceChunkToDevice( { m.dsc.fwType, m.dsc.fwVersion, *nextChunk }, m.proto.connectorId, m.proto );
    }

    void _onMessage( const DataMessage& m ) {
        if ( ! _matchingFWTypeMessage( m ) ) {  // todo: pass lower version too??
            _processIncompatibleMessage( m );
            return;
        }
        _trySatisfyForeignRequests( m.dsc, m.data, m.proto );
        if ( ! _ongoingUpdate ) return;
        if (
             m.dsc.fwVersion != _ongoingUpdate->fwVersion
             || ! _isValidChunkId(
                     _ongoingUpdate->candidateFirmware.size,
                     m.dsc.chunkId,
                     m.proto.chunkSize
                 )
             || _ongoingUpdate->isChunkPresent( m.dsc.chunkId )
        ) return;

        _ongoingUpdate->writeChunk( m.dsc.chunkId, m.data );
        _ongoingUpdate->markProgress();
        _inFlightRequestsStore.markRequestInFlightFor( m.dsc, MYSELF_CONNECTOR_ID, false );
        _announceChunk( m.dsc, { m.proto.connectorId }, m.proto );

        if ( _ongoingUpdate->isComplete() ) {
            _ongoingUpdate->commit();
        }
    }

    bool updating() {
        return _ongoingUpdate.has_value();
    }

    std::optional< Clock::time_point > lastProgressAt() {
        if ( ! updating() ) return {};
        return _ongoingUpdate->lastProgress;
    }

    void announceRunningFirmware() {
        _announceChunk(
                { _runningFirmware.type, _runningFirmware.version, 0 },
                {},
                { -1, _chunkSize, _sizeToChunks( _runningFirmware.size, _chunkSize ), _runningFirmware.size }
        );
    }

    void requestNextMissingChunk() {
        if ( ! updating() ) return;
        const auto nextChunk = _ongoingUpdate->firstMissingChunkId();
        if ( ! nextChunk ) return;

        const auto chunkDescriptor = ChunkDescriptor{ _ongoingUpdate->fwType, _ongoingUpdate->fwVersion, *nextChunk };
        _requestChunkForDevice( MYSELF_CONNECTOR_ID, chunkDescriptor, _ongoingUpdate->proto );
    }

private:
    [[nodiscard]]
    bool _validMessage( const Message& m ) const {
        if ( _proto( m ).chunkSize != _chunkSize )
            return false;

        // todo: check proto version

        return true;
    }

    [[nodiscard]]
    bool _matchingFWTypeMessage( const Message& m ) const {
        return _dsc( m ).fwType == _fwType;
    }

    void _trySatisfyForeignRequests( const ChunkDescriptor& dsc, const std::vector< unsigned char >& data, const Proto& proto ) {
        auto reqs = _inFlightRequestsStore.getRequesters( dsc );

        DataMessage m{ proto, dsc, data };
        reqs.erase(MYSELF_CONNECTOR_ID);
        for ( auto connectorId : reqs ) {
            _inFlightRequestsStore.markRequestInFlightFor( dsc, connectorId, false );
            _sendMessage( connectorId, m );
        }

    }

    void _announceChunk( const ChunkDescriptor& dsc, const std::optional<ConnectorId>& exclude, const Proto& proto ) {
        AnnounceMessage m( proto, dsc );
        _broadcastMessage( m, exclude );
    }

    void _processIncompatibleMessage( const AnnounceMessage& m ) {
        if ( _diffAnnouncesSeenStore.recentlySeen( m.dsc ) ) return;
        _diffAnnouncesSeenStore.markRecentlySeen( m.dsc );
        _announceChunk( m.dsc, { m.proto.connectorId }, m.proto );
    }

    void _processIncompatibleMessage( const RequestMessage& m ) {
        _requestChunkForDevice( m.proto.connectorId, m.dsc, m.proto );
    }

    void _processIncompatibleMessage( const DataMessage& m ) {
        if ( _datasSeenStore.recentlySeen( m.dsc ) ) return;  // end
        _datasSeenStore.markRecentlySeen( m.dsc );
        _trySatisfyForeignRequests( m.dsc, m.data, m.proto );
    }

    void _initUpdate( const Proto& proto, const ChunkDescriptor& dsc ) {
        _ongoingUpdate = std::make_optional< OngoingUpdate >({  // todo: make proper constructor
            .fwType = dsc.fwType,
            .fwVersion = dsc.fwVersion,
            .proto = proto,
            .candidateFirmware = { .version=dsc.fwVersion, .type=dsc.fwType, .size=proto.fwSize },  // todo: remove candidate firmware may not be needed at all
            .lastProgress = Clock::now(),
            ._presentChunks = std::vector< bool >( proto.chunks, false ),
            ._updatePartition = _updatePartition
        });
    }

    void _requestChunkFromDevice( const ConnectorId& id, const ChunkDescriptor& dsc, const Proto& proto ) {
        bool inFlight = _inFlightRequestsStore.isRequestInFlightForAnybody( dsc );
        _inFlightRequestsStore.markRequestInFlightFor( dsc, MYSELF_CONNECTOR_ID );

        if ( ! inFlight ) {
            auto req = RequestMessage( proto, dsc );
            _sendMessage( id, req );
        }

    }

    void _sendData( const ChunkDescriptor& dsc, const ConnectorId& id, hal::Partition& dataSource, const Proto& proto ) {
        FWSize actualChunkSize = _actualChunkSize( dataSource.getSize(), dsc.chunkId, proto.chunkSize );
        std::vector< unsigned char > data( actualChunkSize );

        dataSource.read( dsc.chunkId * proto.chunkSize, actualChunkSize, data );

        auto msg = DataMessage( proto, dsc, std::move( data ) );  // todo: assert that move works
        _sendMessage( id, msg );
    }

    static std::optional< ChunkId > _nextAvailableChunkId( const ChunkDescriptor& dsc, const FWSize& fwSize, const ChunkSize& chunkSize ) {
        auto maxChunkId = _sizeToChunks( fwSize, chunkSize ) - 1;
        if ( dsc.chunkId < maxChunkId ) {
            return dsc.chunkId + 1;
        }
        return {};
    }

    void _announceChunkToDevice( const ChunkDescriptor& dsc, const ConnectorId& id, const Proto& proto ) {
        AnnounceMessage m{ proto, dsc };
        _sendMessage( id, m );
    }

    void _requestChunkForDevice(const ConnectorId& requester, const ChunkDescriptor& dsc, const Proto& proto ) {
        bool inFlight = _inFlightRequestsStore.isRequestInFlightForAnybody( dsc );
        _inFlightRequestsStore.markRequestInFlightFor( dsc, requester );

        if ( ! inFlight ) {
            auto req = RequestMessage( proto, dsc );
            _broadcastMessage( req, { requester } );  // todo: in future we may exclude all requesters for this chunk??
        }
    }

    void _broadcastMessage( const Message& m, const std::optional< ConnectorId > exclude ) {
        for ( ConnectorId id = 0; id < _connectors.size(); ++id ) {
            if ( exclude && id == *exclude ) continue;
            _sendMessage( id, m );
        }
    }


    std::optional< OngoingUpdate > _ongoingUpdate;
    Firmware _runningFirmware;

    // todo: make timeout configurable, and size configurable too
    RecentlySeenStore< ChunkDescriptor, int > _diffAnnouncesSeenStore{ 20 };
    RequestStore< ChunkDescriptor, int, ConnectorId > _inFlightRequestsStore{ 20 };
    RecentlySeenStore< ChunkDescriptor, int > _datasSeenStore{ 20 };

    const std::function< void( const ConnectorId&, const Message& ) > _sendMessage;

    std::vector< ConnectorId > _connectors;

    const FWType _fwType;
    const ChunkSize _chunkSize;
    hal::Partition _runningPartition;
    hal::UpdatePartition _updatePartition;
};

}
