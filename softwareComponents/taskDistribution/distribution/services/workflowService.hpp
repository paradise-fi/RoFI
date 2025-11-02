#include "memoryService.hpp"
#include "functionRegistry.hpp"
#include "loggingService.hpp"
#include "messaging/messageSender.hpp"

class WorkFlowService
{
    MessageSender& _sender;
    FunctionRegistry& _functionRegistry;
    DistributedMemoryService& _memoryService;
    LoggingService& _loggingService;

    void tryDistributeNewTask( int methodId )
    {
        if ( !_functionRegistry.anyTaskRequests() )
        {
            return;
        }

        auto requester = _functionRegistry.getTaskRequester();
        if ( !requester.has_value() )
        {
            return;
        }

        auto task = _functionRegistry.popTaskForAddress( requester.value(), true );
        if ( !task.has_value() )
        {
            _loggingService.logError( "Task distribution failed: No initial task given." );
            return;
        }

        auto function = _functionRegistry.getFunction( task.value().get().functionId() );
        if ( !function.has_value() )
        {
            _loggingService.logError( "Task distribution failed: Given task has associated no function." );
            return;
        }

        distributeTask( function.value().get().distributionType(), task.value().get(), methodId, requester.value() );
    }

    void distributeTask( FunctionDistributionType distributionType, TaskBase& task, 
        int methodId, const Ip6Addr& requester )
    {
        switch ( distributionType )
        {
            case FunctionDistributionType::Broadcast: 
            {
                _sender.broadcastMessage( DistributionMessageType::TaskAssignment, task, methodId );
                return;
            }

            case FunctionDistributionType::Unicast:
            {
                _sender.sendMessage( DistributionMessageType::TaskAssignment, task, requester );
                return;
            }

            default:
            {
                _loggingService.logError( "Undefined function distribution type found." );
                return;
            }
        }
    }
public:
    WorkFlowService(MessageSender& sender, FunctionRegistry& functionRegistry, DistributedMemoryService& memoryService, LoggingService& loggingService )
    : _sender( sender ), _functionRegistry( functionRegistry ), _memoryService( memoryService ), _loggingService( loggingService ) {}

    void doWorkLeader( int methodId )
    {
        tryDistributeNewTask( methodId );
        _memoryService.processQueue();
        _functionRegistry.processTaskResultQueue();
    }

    void doWorkFollower( const Ip6Addr& address, const Ip6Addr& leader )
    {
        _memoryService.processQueue();

        auto taskCandidate = _functionRegistry.popTaskForAddress( address );

        if ( !taskCandidate.has_value() )
        {
            return;
        }

        auto task = std::move( taskCandidate.value() );
        auto fn = _functionRegistry.getFunction( task.get().functionId() );
        
        if ( !fn.has_value() )
        {
            _functionRegistry.finishActiveTask( address );
            std::ostringstream stream;
            stream << "doWorkFollower: Failed to retrieve function with ID " << task.get().functionId();
            _loggingService.logError( stream.str() );
            return;
        }

        bool functionInvocationSucceeded = _functionRegistry.invokeFunction( task.get() );
        TaskStatus status = task.get().status();

        if ( status == TaskStatus::RepeatLocally )
        {
            auto function = _functionRegistry.getFunction( task.get().functionId() );
            _functionRegistry.enqueueTask( address, _functionRegistry.finishAndGetActiveTask( address ), function->get().completionType() );
            return;
        }

        if  ( !functionInvocationSucceeded )
        {
            _sender.sendMessage( DistributionMessageType::TaskFailed, task.get(), leader );

            if ( !fn.has_value() || fn.value().get().completionType() != FunctionCompletionType::Blocking )
            {
                _functionRegistry.finishActiveTask( address );
            }
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskResult, task.get(), leader );
        if ( !fn.has_value() || fn.value().get().completionType() != FunctionCompletionType::Blocking )
        {
            _functionRegistry.finishActiveTask( address );
        }
    }
};