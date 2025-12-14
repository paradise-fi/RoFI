#pragma once

#include <shared_mutex>
#include <queue>
#include <set>
#include "task.hpp"
#include "../functions/functionModel.hpp"
#include "taskScheduler.hpp"
#include "taskResultEntry.hpp"
#include <boost/lockfree/queue.hpp>

class TaskManager
{
    unsigned int _taskId = 1;
    std::unique_ptr< TaskBase > _initialTask;
    boost::lockfree::queue< ip6_addr_t > _taskRequests = boost::lockfree::queue< ip6_addr_t >( 1024 );
    std::deque< TaskResultEntry > _taskResults;
    std::map< Ip6Addr, TaskScheduler > _schedulers;
    mutable std::shared_mutex _mutex;
    std::set< int > _barrierFunctions;

    void updateTaskIdIfStale( unsigned int newId );

    bool enqueueTaskInternal( const Ip6Addr& addr, std::unique_ptr< TaskBase >&& task, FunctionCompletionType completionType );

public:
    void registerBarrierFunction( int functionId );

    bool enqueueTaskResult( std::unique_ptr< TaskBase > task, const Ip6Addr& origin, bool pushToFront = false );

    std::optional< TaskResultEntry > popTaskResult();

    bool enqueueTaskRequest( const Ip6Addr& addr );

    bool popTaskRequest( Ip6Addr& result );

    bool anyTaskRequests();

    bool enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType );

    template< SerializableOrTrivial Result >
    bool enqueueTask( const Ip6Addr& addr, int functionId, FunctionCompletionType completionType, bool enqueueFront = false )
    {
        auto task = Task< Result >( ++_taskId, TaskStatus::Pending, functionId, enqueueFront );
        return enqueueTaskInternal( addr, std::make_unique< Task< Result > >( task ), completionType );
    }

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    bool enqueueTask( const Ip6Addr& addr, int functionId, int priority, bool enqueueFront, FunctionCompletionType completionType, std::tuple< Arguments... >&& arguments )
    {
        auto task = Task< Result, Arguments... >( ++_taskId, TaskStatus::Pending, functionId, priority, enqueueFront, arguments );
        return enqueueTaskInternal( addr, std::make_unique< Task< Result, Arguments... > >( task ), completionType );
    }

    bool enqueueTask( const Ip6Addr& addr, const FunctionConcept& relatedFunction, const uint8_t* buffer );

    bool setInitialTask( const FunctionConcept& relatedFunction );

    std::optional< std::reference_wrapper< TaskBase > > getInitialTask();

    std::optional< std::reference_wrapper< TaskBase > > popTask( const Ip6Addr& address, bool isLeader = false );

    void finishActiveTask( const Ip6Addr& address );

    void finishActiveTask( const Ip6Addr& address, unsigned int taskId );

    std::unique_ptr< TaskBase > finishAndGetActiveTask ( const Ip6Addr& address );

    void clearTasks();

    void unblockSchedulers( bool hardUnblock = false );

    void unblockScheduler( const Ip6Addr& address, bool hardUnblock = false );
};