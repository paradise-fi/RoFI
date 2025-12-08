#include "../../include/distributedTaskManager.hpp"

DistributedTaskManager::DistributedTaskManager(
    std::unique_ptr< ElectionProtocolBase > election,
    Ip6Addr& address,
    MessageDistributor& distributor,
    std::unique_ptr< udp_pcb > pcb,
    int blockingMessageTimeoutMs ) 
: _address( address ),
  _functionRegistry( _loggingService ),
  _election( std::move( election ) ),
  _callbackService( _election, _loggingService ),
  _messaging( address, DISTRIBUTION_PORT, distributor, std::move( pcb ), 
               _messageQueueManager, METHOD_ID ),
  _memoryService( distributor, _messaging, address, _loggingService, 
                  _callbackService, blockingMessageTimeoutMs ),
  _customMessageQueueManager( *this, _messaging ),
  _messageDispatcher( address, *this, _functionRegistry, _messaging, 
                      _memoryService, _loggingService, _customMessageQueueManager, 
                      _messageQueueManager, blockingMessageTimeoutMs ),
  _workFlowService( _messaging.sender(), _functionRegistry, _memoryService, 
                    _loggingService, _customMessageQueueManager, _messageDispatcher ),
  _blockingMessageTimeoutMs( blockingMessageTimeoutMs )
{}

CallbackFacade& DistributedTaskManager::callbacks()
{
    return _callbackService;
}

[[nodiscard]] MemoryFacade DistributedTaskManager::memory()
{
    return MemoryFacade( _memoryService );
}

[[nodiscard]] FunctionFacade DistributedTaskManager::functions()
{
    return FunctionFacade( _functionRegistry );
}

LoggingService& DistributedTaskManager::loggingService()
{
    return _loggingService;
}

void DistributedTaskManager::doWork()
{
    if ( !_election.isRunning() )
    {
        _loggingService.logError("Election protocol not running.");
        return;
    }

    if ( !_election.isElectionComplete() )
    {
        return;
    }

    if ( _address == _election.getLeader() )
    {
        _workFlowService.doWorkLeader( METHOD_ID );
        return;
    }
    
    _workFlowService.doWorkFollower( _address, _election.getLeader() );
}

void DistributedTaskManager::start( int initialElectionDelay, int electionCyclesBeforeStabilization )
{
    _election.start( initialElectionDelay,
        [ this ]( const Ip6Addr& leader) {
            onElectionSuccesful( leader );
        }, electionCyclesBeforeStabilization);
}

std::optional< Ip6Addr > DistributedTaskManager::getLeader()
{
    if ( _election.isElectionComplete() )
    {
        return _election.getLeader();
    }

    return std::nullopt;
}

void DistributedTaskManager::sendCustomMessage( uint8_t* data, size_t dataSize, std::optional< Ip6Addr > target )
{
    if ( !target.has_value() )
    {
        return _messaging.sender().broadcastMessage( DistributionMessageType::CustomMessage, data, dataSize, METHOD_ID );
    }

    auto packet = PBuf::allocate( static_cast< int >( dataSize ) );
    std::memcpy( packet.payload(), data, dataSize );
    _messaging.sender().sendMessage( DistributionMessageType::CustomMessage, std::move( packet ), target.value() );
}

MessagingResult DistributedTaskManager::sendCustomMessageBlocking( uint8_t* data, size_t dataSize, Ip6Addr& target )
{
    return _messaging.sendMessageBlocking( target, DistributionMessageType::CustomMessageBlocking, data, dataSize, _blockingMessageTimeoutMs );
}

bool DistributedTaskManager::requestTask()
{
    if ( _election.getLeader() == _address )
    {
        return false;
    }

    auto emptyTask = Task< int >( 0 );
    _messaging.sender().sendMessage( DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
    return true;
}

void DistributedTaskManager::broadcastUnblockSignal()
{
    _functionRegistry.unblockTaskSchedulers( true );
    _messaging.sender().broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
}

void DistributedTaskManager::sendUnblockSignal( const Ip6Addr& receiver )
{
    _functionRegistry.unblockTaskScheduler( receiver, true );
    _messaging.sender().sendMessage(DistributionMessageType::BlockingTaskRelease, receiver );
}


// ===================== PRIVATE
void DistributedTaskManager::onElectionSuccesful( const Ip6Addr& leader )
{
    if ( _memoryService.isMemoryRegistered() )
    {
        _memoryService.setLeader( _election.getLeader() ); 
    }

    if ( _address == leader )
    {
        _loggingService.logInfo( "I have been elected as the leader." );
        return;
    }

    _functionRegistry.clearTasks();
    std::ostringstream stream;
    stream << "I am a follower of " << leader;
    _loggingService.logInfo( stream.str() );
    requestTask();
}

MessagingResult DistributedTaskManager::invokeUserCallback( CallbackType cbType,
    const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
{
    int taskId;
    bool stopRequestPipeline;
    switch ( cbType )
    {
        case CallbackType::CompleteBlockingMessageCb:
            _messaging.completeBlockingMessage( data, size );
            return MessagingResult( true );
        case CallbackType::CustomMessageCb:
            _callbackService.invokeOnCustomMessage( *this, sender, data, size );
            return MessagingResult( true );
        case CallbackType::CustomMessageBlockingCb:
            return _callbackService.invokeOnCustomMessageBlocking( *this, sender, data, size );
        case CallbackType::TaskFailureCb:
            taskId = 0;
            if ( size == sizeof( int ) )
            {
                taskId = as< int >( data );
            }
            _callbackService.invokeOnTaskFailure( *this, sender, taskId );
            return MessagingResult( true );

        case CallbackType::TaskRequestCb:
            // Semantics -> true means that the task request pipeline ends with this call. False means we still need to queue the task request.
            stopRequestPipeline = _callbackService.invokeOnTaskRequest( *this, sender );
            return MessagingResult( stopRequestPipeline );

        default:
            std::ostringstream errorMessageStream;
            errorMessageStream << "Callback type not registered. " << std::endl;
            _loggingService.logError( errorMessageStream.str() );
            return MessagingResult( false );
    }

    return MessagingResult( false );
}