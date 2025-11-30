#pragma once
#include "lwip++.hpp"
#include "../functionRegistry.hpp"
#include "../services/memoryService.hpp"
#include "../callbacks/userCallbackInvoker.hpp"
#include "customMessageQueueManager.hpp"
#include "messageQueueManager.hpp"
#include <functional>
#include <queue>

class MessageDispatcher
{
    rofi::hal::Ip6Addr& _addr;
    UserCallbackInvoker& _callbackInvoker;
    FunctionRegistry& _functionRegistry;
    MessagingService& _messagingService;
    DistributedMemoryService& _memoryService;
    LoggingService& _loggingService;
    CustomMessageQueueManager& _customMessageQueueManager;
    MessageQueueManager& _messageQueueManager;
    int _blockingMessageTimeoutMs;


    void handleCustomMessage( const Ip6Addr& sender, uint8_t* data, size_t size )
    {
        _callbackInvoker.invokeUserCallback( CallbackType::CustomMessageCb, sender, data, size );
    }

    void handleTaskRequest( const rofi::hal::Ip6Addr& sender )
    {
        // True -> Enqueueing was done by the user callback, pipeline does not continue.
        if ( _callbackInvoker.invokeUserCallback( CallbackType::TaskRequestCb, sender, nullptr, 0 ).success )
        {
            return;
        }
            
        if ( !_functionRegistry.enqueueTaskRequest( sender ) )
        {
            _loggingService.logError( "Failed to enqueue task request." );
        }
        return;   
    }

    void handleMemoryMessage( const Ip6Addr& sender, const DistributionMessageType type, uint8_t* data, size_t size )
    {
        if ( _memoryService.isMemoryRegistered() )
            {
                return _memoryService.onMemoryMessage( sender, data, size, mapMessageToMemoryRequest( type ) );
            }
        else
            {
                _loggingService.logError("Memory not registered.");
            }   
    }

    void handleTaskFailure( const Ip6Addr& sender, int functionId )
    {
        std::vector< uint8_t > messageBuffer;
        messageBuffer.resize( sizeof( int ) );
        std::memcpy( messageBuffer.data(), &functionId, sizeof( int ) );
        _callbackInvoker.invokeUserCallback( CallbackType::TaskFailureCb, sender, messageBuffer.data(), messageBuffer.size() );
    }

    void handleTaskAssignment( std::unique_ptr< TaskBase > task, std::reference_wrapper< FunctionConcept > fn )
    {
        std::ostringstream stream;
        stream << "Task Assignment for task with ID " << task->id();
        _loggingService.logInfo( stream.str() );
        
        if ( !_functionRegistry.enqueueTask( _addr, std::move( task ), fn.get().completionType() ) )
        {
            std::ostringstream failStream;
            failStream << "Failed to register task assignment for function " << fn.get().functionId();
            _loggingService.logError( failStream.str() );
        }
    }

    void handleTaskResult( const rofi::net::Ip6Addr& sender, 
        std::unique_ptr< TaskBase > task, std::reference_wrapper< FunctionConcept > fn )
    {
        int taskId = task->id();

        std::ostringstream stream;
        stream << "Result for Task with ID " << taskId;
        _loggingService.logInfo( stream.str() );

        if ( task->status() == TaskStatus::RepeatDistributed )
        {
            if ( !_functionRegistry.enqueueTask( sender, std::move( task ), fn.get().completionType() ) )
            {
                std::ostringstream failStream;
                failStream << "Failed to register task for repeat for function " << fn.get().functionId();
                _loggingService.logError( failStream.str() );
                return;
            }
            
            if ( !_functionRegistry.enqueueTaskRequest( sender ) )
            {
                _loggingService.logError( "Failed to enqueue task request." );
            }

            return;
        }

        if ( !_functionRegistry.enqueueTaskResult( std::move( task ), sender ) )
        {
            std::ostringstream failStream;
            failStream << "Failed to persist result from task " << taskId << " for function " << fn.get().functionId();
            _loggingService.logError( failStream.str() );
        }
    }

    void handleTaskMessage( const Ip6Addr& sender, const DistributionMessageType type, uint8_t* data )
    {
        if ( !IsMessageTypeTaskMessage( type ) )
        {
            std::ostringstream invalidMessageTypeStream;
            invalidMessageTypeStream << "Unhandled Message Type Detected: " << getMessageTypeName( type );
            _loggingService.logError( invalidMessageTypeStream.str() );
            return;
        }

        if ( type == DistributionMessageType::TaskRequest )
        {
            return handleTaskRequest( sender );
        }

        int functionId = as< int >( data );

        if ( type == DistributionMessageType::TaskFailed )
        {
            return handleTaskFailure( sender, functionId );
        }

        std::optional<std::reference_wrapper< FunctionConcept > > fn = _functionRegistry.getFunction( functionId );

        if ( !fn )
        {
            std::ostringstream stream;
            stream << "Function with ID " << functionId << " not found.";
            _loggingService.logError( stream.str() );
            return;
        }

        auto task = _functionRegistry.getTaskFromBuffer( data, functionId );

        if ( task == nullptr )
        {
            std::ostringstream stream;
            stream << "Unable to parse task for function with ID " << functionId << ".";
            _loggingService.logError( stream.str() );
            return;
        }

        if ( type == DistributionMessageType::TaskAssignment )
        {
            return handleTaskAssignment( std::move( task ), fn.value() );
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            return handleTaskResult( sender, std::move( task ), fn.value() );
        }
    }
public:
    MessageDispatcher( rofi::hal::Ip6Addr& address, UserCallbackInvoker& callbackInvoker, FunctionRegistry& functionRegistry,
        MessagingService& messagingService, DistributedMemoryService& memoryService, LoggingService& loggingService, 
        CustomMessageQueueManager& customMessageQueueManager, MessageQueueManager& messageQueueManager, int blockingMessageTimeoutMS )
    : _addr( address ), _callbackInvoker( callbackInvoker ), _functionRegistry( functionRegistry ), _messagingService( messagingService ),
      _memoryService( memoryService ), _loggingService( loggingService ), _customMessageQueueManager( customMessageQueueManager ),
      _messageQueueManager( messageQueueManager ), _blockingMessageTimeoutMs( blockingMessageTimeoutMS ) { }

    bool dispatchMessageFromQueue()
    {
        if ( _messageQueueManager.isEmpty() )
        {
            return false;
        }

        std::optional< MessageEntry > result = _messageQueueManager.popMessage();

        if ( !result.has_value() )
        {
            return false;
        }

        dispatchMessage( result.value().sender, result.value().messageType, 
                         result.value().rawData.data(), result.value().rawData.size() );

        return true;        
    }

    void dispatchMessage( const Ip6Addr& sender, DistributionMessageType messageType, uint8_t* data, size_t size )
    {
        std::ostringstream receivedMessageStream;
        receivedMessageStream << "Received " << getMessageTypeName( messageType ) << " from " << sender;
        _loggingService.logInfo( receivedMessageStream.str() );

        switch ( messageType )
        {
            case DistributionMessageType::CustomMessage:
                return handleCustomMessage( sender, data, size );
            
            case DistributionMessageType::CustomMessageBlocking:
                _customMessageQueueManager.emplaceRequest( sender, data, size );
                break;

            case DistributionMessageType::BlockingTaskRelease:
                _functionRegistry.unblockTaskSchedulers( true );
                break;

            case DistributionMessageType::BlockingMessageResponse:
                _messagingService.completeBlockingMessage( data, size );
                break;

            case DistributionMessageType::DataStorageRequest:
            case DistributionMessageType::DataReadRequest:
            case DistributionMessageType::DataReadRequestBlocking:
            case DistributionMessageType::DataRemovalRequest:
                handleMemoryMessage( sender, messageType, data, size );
                break;

            default:
                handleTaskMessage( sender, messageType, data );
                break;
        };
    }
};