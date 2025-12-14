#include "memoryMessagingWrapper.hpp"

MemoryMessagingWrapper::MemoryMessagingWrapper( MessagingService& messagingService, LoggingService& loggingService, unsigned int methodId, rofi::hal::Ip6Addr& currentAddress ) 
: _messaging( messagingService ), _loggingService( loggingService ), _methodId( methodId ), _currentAddress( currentAddress ) {}

void MemoryMessagingWrapper::setLeader( const rofi::hal::Ip6Addr& leaderAddr )
{
    _leader = leaderAddr;
}

const rofi::hal::Ip6Addr& MemoryMessagingWrapper::getLeader()
{
    return _leader;
}

bool MemoryMessagingWrapper::isLeaderMemory()
{
    return _leader == _currentAddress;
}

void MemoryMessagingWrapper::unregisterMemory()
{
    _memory = nullptr;
}

void MemoryMessagingWrapper::registerMemory( DistributedMemoryBase* memory )
{
    _memory = memory;
}

size_t MemoryMessagingWrapper::genericMemoryMessageHeaderSize()
{
    return sizeof( int ) + sizeof( bool ) + sizeof( size_t );
}

size_t MemoryMessagingWrapper::genericMessageHeaderSize()
{
    return _messaging.sender().headerSize();
}

void MemoryMessagingWrapper::prepareMemoryMessageData( uint8_t* buffer, int address, const uint8_t* data, size_t dataSize, bool isMetadataOnly )
{
    as< int >( buffer ) = address;
    as< bool >( buffer + sizeof( int ) ) = isMetadataOnly;
    as< size_t >( buffer + sizeof( int ) + sizeof( bool ) ) = dataSize;

    if ( dataSize != 0 && data != nullptr )
    {
        std::memcpy( buffer + genericMemoryMessageHeaderSize(), data, dataSize );
    }
}


void MemoryMessagingWrapper::sendDataBroadcast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, DistributionMessageType messageType )
{
    // ADDRESS - IS METADATA ONLY - DATA SIZE - DATA
    std::vector< uint8_t > buffer;

    buffer.resize( size + genericMemoryMessageHeaderSize() );
    prepareMemoryMessageData( buffer.data(), address, data, size, isMetadataOnly );

    _messaging.sender().broadcastMessage( messageType, buffer.data(), buffer.size(), _methodId );
}

void MemoryMessagingWrapper::sendDataUnicast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, const Ip6Addr& target, DistributionMessageType messageType )
{
    PBuf packet = PBuf::allocate( static_cast< int >( genericMemoryMessageHeaderSize() + size ) );
    prepareMemoryMessageData( packet.payload(), address, data, size, isMetadataOnly );
    auto sendResult = _messaging.sender().sendMessage( messageType, std::move( packet ), target );

    if ( !sendResult.success )
    {
        std::ostringstream stream;
        stream << " Memory Messaging Failure to " << target << ", reason: " << sendResult.messsage;
        _loggingService.logError( stream.str() );
    }
}

void MemoryMessagingWrapper::propagateMemoryChange( const MemoryPropagationType type, int address, uint8_t* data, size_t size,
    bool isMetadataOnly, std::optional< Ip6Addr > target, DistributionMessageType messageType )
{
    switch ( type )
    {
        case MemoryPropagationType::ONE_TARGET:
            sendDataUnicast( address, data, size, isMetadataOnly, target.has_value() ? target.value() : _leader, messageType );
            break;
        case MemoryPropagationType::SEND_TO_ALL:
            sendDataBroadcast( address, data, size, isMetadataOnly, messageType );
            break;
        default:
            return;
    }
}

void MemoryMessagingWrapper::propagateMetadataChange( MemoryPropagationType type, int address, const std::string& key, uint8_t* metadata, 
    size_t metadataSize, std::optional< Ip6Addr > target, DistributionMessageType messageType)
{
    if ( _memory == nullptr )
    {
        _loggingService.logWarning("Tried to propagate metadata change in memory but no memory implementation is registered.");
        return;
    }

    std::size_t keySize = key.size();

    std::vector< uint8_t > dataBuffer;
    if ( metadata != nullptr )
    {
        dataBuffer = _memory->serializeDataToMemoryFormat( metadata, metadataSize, address, true );
    }

    size_t originalDataSize = dataBuffer.size();
    dataBuffer.resize( key.size() + dataBuffer.size() + sizeof( size_t ) );

    // Move data to the back of the buffer.
    std::memmove( dataBuffer.data() + key.size() + sizeof( size_t ), dataBuffer.data(), originalDataSize );
    
    // Key
    std::memcpy( dataBuffer.data(), &keySize, sizeof( size_t ) );
    if ( keySize > 0 )
    {
        std::memcpy( dataBuffer.data() + sizeof( size_t ), key.data(), key.size() );
    }
    
    // std::memcpy( dataBuffer.data() + sizeof( size_t ) + keySize, dataBuffer.data(), dataBuffer.size() );
    
    propagateMemoryChange( type, address, dataBuffer.data(), dataBuffer.size(), true, target, messageType );
}

bool MemoryMessagingWrapper::composeReadRequest( uint8_t* buffer, size_t bufferSize, const Ip6Addr& origin, int address )
{
    if ( bufferSize < readRequestBufferSize() )
    {
        return false;
    }

    // Requester Address
    as< Ip6Addr >( buffer ) = origin;
    as< u8_t >( buffer + sizeof( Ip6Addr ) ) = origin.zone;

    // Data Address
    as< int >( buffer + sizeof( Ip6Addr ) + sizeof(u8_t) ) = address;
    
    // IsMetadataRequest
    as< bool >( buffer + sizeof( int ) + sizeof( Ip6Addr ) + sizeof(u8_t)  ) = false;

    // Size of other data.
    as< size_t >( buffer + sizeof( int ) + sizeof( bool ) + sizeof( Ip6Addr ) + sizeof(u8_t) ) = 0;

    return true;
}

bool MemoryMessagingWrapper::composeMetadataReadRequest( uint8_t* buffer, size_t bufferSize, const Ip6Addr& origin, int address, const std::string& key )
{
    if ( bufferSize < readRequestBufferSize() + key.size() )
    {
        return false;
    }

    // Requester Address
    as< Ip6Addr >( buffer ) = origin;
    as< u8_t >( buffer + sizeof( Ip6Addr ) ) = origin.zone;

    // Data Address
    as< int >( buffer + sizeof( Ip6Addr ) + sizeof( u8_t ) ) = address;

    // IsMetadataRequest -> TRUE
    as< bool >( buffer + sizeof( int ) + sizeof( Ip6Addr ) + sizeof( u8_t ) ) = true;

    // Size of Metadata Key
    as< size_t >( buffer + sizeof( int ) + sizeof( bool ) + sizeof( Ip6Addr ) + sizeof( u8_t )  ) = key.size();

    // Key
    if ( key.size() > 0 )
    {
        std::memcpy( buffer + readRequestBufferSize(), key.data(), key.size() );    
    }

    return true;
}

size_t MemoryMessagingWrapper::readRequestBufferSize()
{
    return sizeof( Ip6Addr ) + sizeof( u8_t ) + sizeof( int ) + sizeof( bool ) + sizeof( size_t );
}