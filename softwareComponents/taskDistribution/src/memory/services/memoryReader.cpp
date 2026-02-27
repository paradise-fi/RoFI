#include "memoryReader.hpp"

MemoryReader::MemoryReader( MemoryMessagingWrapper& memoryMessaging, rofi::hal::Ip6Addr currentModuleAddress, 
    MessagingService& messaging, LoggingService& logging, int blockingMessageTimeoutMs ) 
: _memoryMessaging( memoryMessaging ), _currentModuleAddress( currentModuleAddress ), 
  _messaging( messaging ), _loggingService( logging ), _blockingMessageTimeoutMs( blockingMessageTimeoutMs ) {}

/// @brief Reads data from specified address in memory.
/// @param address The memory address to be read
/// @return MemoryReadResult struct containing information about read success and a method for extracting the data read.
MemoryReadResult MemoryReader::readData( int address, bool isUserCall )
{
    if ( _memory == nullptr )
    {
        return MemoryReadResult{ false, false, std::nullopt, std::vector< uint8_t >() };
    }
    return readDataInternal( address, _currentModuleAddress, isUserCall );
}
    
MemoryReadResult MemoryReader::readMetadata( int address, const std::string& key, bool isUserCall )
{
    if ( _memory == nullptr )
    {
        return MemoryReadResult{ false, false, std::nullopt, std::vector< uint8_t >() };
    }

    return readMetadataInternal( address, _currentModuleAddress, key, isUserCall );
}

void MemoryReader::unregisterMemory()
{
    _memory = nullptr;
}

void MemoryReader::registerMemory( DistributedMemoryBase* memory )
{
    _memory = memory;
}

void MemoryReader::processRemoteDataReadRequest( uint8_t* data, size_t size )
{
    // This header contains the address of the original sender in the case of message forwarding.
    size_t headerSize = _memoryMessaging.genericMemoryMessageHeaderSize() + sizeof( Ip6Addr );

    if ( size < headerSize )
    {
        _loggingService.logError( "Distributed Memory - Malformed message deceted. Data not saved." );
        return;
    }

    size_t offset = 0;

    Ip6Addr sender = as< Ip6Addr >( data );
    sender.zone = as< u8_t >( data + sizeof( Ip6Addr ) );

    offset += sizeof( Ip6Addr ) + sizeof( u8_t );

    int address = as< int >( data + offset );
    offset += sizeof( int );

    bool isMetadataOnly = as< bool >( data + offset );
    offset += sizeof( bool );

    handleRemoteDataReadRequest( sender, data + offset, size - offset, address, isMetadataOnly );
}

// =============== PRIVATE
void MemoryReader::handleRemoteDataReadRequest( const Ip6Addr& sender, uint8_t* data, size_t dataSize,
    int address, bool isMetadataOnly )
{
    if ( isMetadataOnly )
    {
        std::string key;
        // Extract key
        if ( dataSize > 0 )
        {
            size_t keySize = as< size_t >( data );
            if ( keySize > 0 )
            {
                key.resize( keySize );
                std::memcpy( key.data(), data + sizeof( size_t ), keySize );
            }
        }

        readMetadataInternal( address, sender, key, false ); 
    }
    else
    {
        readDataInternal( address, sender, false );
    }
}

MemoryReadResult MemoryReader::readDataBlocking( const Ip6Addr& target, uint8_t* data, size_t dataSize )
{
    MemoryReadResult result;
    result.success = false;
    auto remoteReadResult = _messaging.sendMessageBlocking( target, DistributionMessageType::DataReadRequestBlocking, data, dataSize, _blockingMessageTimeoutMs );
    if ( remoteReadResult.success )
    {
        auto success = as< bool >( remoteReadResult.rawData.data() + _memoryMessaging.genericMemoryMessageHeaderSize() );
        size_t readSize = as< size_t >( remoteReadResult.rawData.data() + sizeof( bool ) + _memoryMessaging.genericMemoryMessageHeaderSize() );
        
        if ( success )
        {
            result.rawData.resize( readSize );
            std::memcpy( result.rawData.data(), remoteReadResult.rawData.data() + sizeof( bool ) + sizeof( size_t ) + _memoryMessaging.genericMemoryMessageHeaderSize(), readSize );
            result.success = true;
        }
    }
    else
    {
        std::ostringstream stream;
        stream << "MemoryService - Remote Data Retrieval at " << target << " failed, reason: " << remoteReadResult.statusMessage;
        _loggingService.logError( stream.str() );
    }

    return result;
}

MemoryReadResult MemoryReader::forwardReadRequest( const Ip6Addr& target, uint8_t* data, size_t dataSize )
{
    MemoryReadResult result;
    result.success = true;
    result.requestRemoteRead = true;
    auto remoteReadRequestResult = _messaging.sender().sendMessage( DistributionMessageType::DataReadRequest, data, dataSize, target );
    
    if ( !remoteReadRequestResult.success )
    {
        result.success = false;
        std::ostringstream stream;
        stream << "MemoryService - Remote Data Retrieval relay to " << target << " failed, reason: " << remoteReadRequestResult.messsage;
        _loggingService.logError( stream.str() );
    }

    return result;
}

MemoryReadResult MemoryReader::readDataInternal( int address, const Ip6Addr& origin, bool isUserCall )
{
    auto result = _memory->readData( address, _memoryMessaging.getLeader() == _currentModuleAddress );

    std::vector< uint8_t > data;
    if ( !result.requestRemoteRead && isUserCall )
    {
        return result;
    }

    if ( !result.requestRemoteRead && !isUserCall )
    {
        data.resize( result.rawData.size() + sizeof( size_t ) + sizeof( bool ) );
        as< bool >( data.data() ) = result.success;
        as< size_t >( data.data() + sizeof( bool ) ) = result.success ? result.rawData.size() : 0;
        if ( result.success )
        {
            std::memcpy( data.data() + sizeof( bool ) + sizeof( size_t ), result.rawData.data(), result.rawData.size() );
        }

        // Send a message to the origin.
        _memoryMessaging.sendDataUnicast( address, data.data(), data.size(), false, origin, DistributionMessageType::BlockingMessageResponse );
        return result;
    }

    result.success = false;
    const Ip6Addr& target = result.readTarget.has_value() ? result.readTarget.value() : _memoryMessaging.getLeader();
    data.resize( _memoryMessaging.readRequestBufferSize() );

    if ( !_memoryMessaging.composeReadRequest( data.data(), data.size(), origin, address ) )
    {
        _loggingService.logError("MemoryReader: Failed to compose data read request message.");
        return result;
    }

    if ( isUserCall ) 
    {
        return readDataBlocking( target, data.data(), data.size() );
    }

    auto forwardResult = forwardReadRequest( target, data.data(), data.size() );

    if ( !forwardResult.success )
    {
        std::vector< uint8_t > errorMessage;
        errorMessage.resize( sizeof( bool ) + sizeof( size_t ) );
        as< bool >( errorMessage.data() ) = false;
        as< size_t >( errorMessage.data() + sizeof( bool ) ) = 0;
        auto messageResult = _messaging.sender().sendMessage( DistributionMessageType::BlockingMessageResponse, errorMessage.data(), errorMessage.size(), origin );
        if ( !messageResult.success )
        {
            std::ostringstream stream;
            stream << "MemoyService - Error while fetching data for blocking read for " << origin << ", reason: " << messageResult.messsage;
            _loggingService.logError( stream.str() );
        }
    }

    return forwardResult;
}

MemoryReadResult MemoryReader::readMetadataInternal( int address, const Ip6Addr& origin, const std::string& key, bool isUserCall )
{
    auto result = _memory->readMetadata( address, key, _memoryMessaging.getLeader() == _currentModuleAddress );
    std::vector< uint8_t > data;

    if ( !result.requestRemoteRead && isUserCall )
    {
        return result;
    }

    if ( !result.requestRemoteRead && !isUserCall )
    {
        data.resize( result.rawData.size() + sizeof( size_t ) + sizeof( bool ) );
        as< bool >( data.data() ) = result.success;
        as< size_t >( data.data() + sizeof( bool ) ) = result.success ? result.rawData.size() : 0;
        if ( result.success )
        {
            std::memcpy( data.data() + sizeof( bool ) + sizeof( size_t ), result.rawData.data(), result.rawData.size() );
        }

        // Send a message to the origin.
        _memoryMessaging.sendDataUnicast( address, data.data(), data.size(), true, origin, DistributionMessageType::BlockingMessageResponse );
        return result;
    }

    result.success = false;
    const Ip6Addr& target = result.readTarget.has_value() ? result.readTarget.value() : _memoryMessaging.getLeader();
    
    data.resize( _memoryMessaging.readRequestBufferSize() + key.size() );
    if ( !_memoryMessaging.composeMetadataReadRequest( data.data(), data.size(), origin, address, key ) )
    {
        _loggingService.logError("MemoryReader: Failed to compose metadata read request message.");
        return result;
    }

    return isUserCall 
        ? readDataBlocking( target, data.data(), data.size() )
        : forwardReadRequest( target, data.data(), data.size() );
}