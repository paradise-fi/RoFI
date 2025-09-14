#include "functionRegistry.hpp"
#include "messaging/messageSender.hpp"

class WorkFlowService
{
    MessageSender& _sender;
    FunctionRegistry& _functionRegistry;

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
            std::cout << "Task distribution failed: No initial task given.";
            return;
        }

        auto function = _functionRegistry.getFunction( task.value().get().functionId() );
        if ( !function.has_value() )
        {
            std::cout << "Task distribution failed: Given task has associated no function.";
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
                std::cout << "Undefined Function Distribution Type found." << std::endl;
                return;
            }
        }
    }
public:
    WorkFlowService(MessageSender& sender, FunctionRegistry& functionRegistry)
    : _sender( sender ), _functionRegistry( functionRegistry ){}

    void doWorkLeader( int methodId )
    {
        tryDistributeNewTask( methodId );
        _functionRegistry.processTaskResultQueue();
    }

    void doWorkFollower( const Ip6Addr& address, const Ip6Addr& leader )
    {
        auto taskCandidate = _functionRegistry.popTaskForAddress( address );

        if ( !taskCandidate.has_value() )
        {
            return;
        }

        auto task = std::move( taskCandidate.value() );
        
        if  ( !_functionRegistry.invokeFunction( task.get() ) )
        {
            _sender.sendMessage( DistributionMessageType::TaskFailed, task.get(), leader );
            _functionRegistry.finishActiveTask( address );
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskResult, task.get(), leader );
        _functionRegistry.finishActiveTask( address );
    }
};