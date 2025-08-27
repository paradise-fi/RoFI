#pragma once

#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskScheduler.hpp"
#include <boost/lockfree/queue.hpp>

class TaskManager
{
    // ToDo: This int should be atomic!
    int _taskId = 1;
    std::unique_ptr< TaskBase > _initialTask;
    boost::lockfree::queue< ip6_addr_t > _taskRequests = boost::lockfree::queue< ip6_addr_t >( 1024 );
    std::map< Ip6Addr, TaskScheduler > _schedulers;

    void updateTaskIdIfStale( int newId )
    {
        _taskId = _taskId < newId ? newId : _taskId;
    }

public:
    void registerBarrierFunction( int functionId )
    {
        for( auto it = _schedulers.begin(); it != _schedulers.end(); ++it)
        {
            it->second.registerBarrier( functionId );
        }
    }

    void enqueueTaskRequest( const Ip6Addr& addr )
    {
        _taskRequests.push( addr );
    }

    bool popTaskRequest( Ip6Addr& result )
    {
        return _taskRequests.pop( result );
    }

    bool anyTaskRequests()
    {
        return !_taskRequests.empty();
    }

    bool enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType )
    {
        _schedulers[ addr ].enqueueTask( std::move( task ), completionType );
        return true;
    }

    template< typename Result >
    bool enqueueTask( Ip6Addr addr, int functionId, FunctionCompletionType completionType, bool enqueueFront = false )
    {
        auto task = Task< Result >( ++_taskId, TaskStatus::Enqueued, functionId, enqueueFront );
        _schedulers[ addr ].enqueueTask( std::make_unique< TaskBase >( task ), completionType );

        return true;
    }

    template < typename Result, typename... Arguments >
    bool enqueueTask( Ip6Addr addr, int functionId, int priority, bool enqueueFront, FunctionCompletionType completionType, std::tuple< Arguments... >&& arguments )
    {
        auto task = Task< Result, Arguments... >( ++_taskId, TaskStatus::Enqueued, functionId, priority, enqueueFront, arguments );
        _schedulers[ addr ].enqueueTask( std::make_unique< Task< Result, Arguments... > >( task ), completionType );

        return true;
    }

    bool enqueueTask( Ip6Addr addr, const FunctionConcept& relatedFunction, const uint8_t* buffer )
    {
        auto task = relatedFunction.createTask();
        task->fillFromBuffer( buffer );
        updateTaskIdIfStale( task->id() );
        _schedulers[ addr ].enqueueTask( std::move( task ), relatedFunction.completionType() );
        return true;
    }

    bool setInitialTask( const FunctionConcept& relatedFunction )
    {
        auto task = relatedFunction.createTask();
        _initialTask = std::move( task );

        return true;
    }

    std::optional< std::reference_wrapper< TaskBase > > getInitialTask()
    {
        
        if ( _initialTask == nullptr )
        {
            std::cout << "Initial task is null" << std::endl;
            return std::nullopt;
        }
        
        return std::optional< std::reference_wrapper< TaskBase > >( *_initialTask );
    }

    std::optional< std::reference_wrapper< TaskBase > > popTask( Ip6Addr& address, bool isLeader = false )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            std::cout << "Address not found in tasks" << std::endl;
            return std::nullopt;
        }

        return taskQueue->second.popTask( isLeader );
    }

    void finishActiveTask( Ip6Addr& address )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            std::cout << "Address not found in tasks" << std::endl;
            return;
        }

        taskQueue->second.clearActiveTask();
    }

    void finishActiveTask( Ip6Addr& address, int id )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            std::cout << "Address not found in tasks" << std::endl;
            return;
        }

        taskQueue->second.clearActiveTask( id );
    }

    void unblockSchedulers()
    {
        for( auto it = _schedulers.begin(); it != _schedulers.end(); ++it)
        {
            it->second.clearActiveTask();
        }
    }

    void clearTasks()
    {
        unblockSchedulers();
        _schedulers.clear();
    }
};