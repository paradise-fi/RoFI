#pragma once

#include <shared_mutex>
#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskScheduler.hpp"
#include "taskResultEntry.hpp"
#include <boost/lockfree/queue.hpp>

class TaskManager
{
    // ToDo: This int should be atomic!
    int _taskId = 1;
    std::unique_ptr< TaskBase > _initialTask;
    boost::lockfree::queue< ip6_addr_t > _taskRequests = boost::lockfree::queue< ip6_addr_t >( 1024 );
    std::deque< TaskResultEntry > _taskResults;
    std::map< Ip6Addr, TaskScheduler > _schedulers;
    mutable std::shared_mutex _mutex;

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

    bool enqueueTaskResult( std::unique_ptr< TaskBase > task, const Ip6Addr& origin, bool pushToFront = false )
    {
        std::unique_lock lock( _mutex );
        if ( pushToFront )
        {
            _taskResults.push_front( TaskResultEntry{ std::move( task ), origin } );
        }
        else
        {
            _taskResults.push_back( TaskResultEntry{ std::move( task ), origin } );
        }
        return true;
    }

    std::optional< TaskResultEntry > popTaskResult()
    {
        std::unique_lock lock( _mutex );
        if ( _taskResults.empty() )
        {
            return std::nullopt;
        }

        TaskResultEntry result = std::move( _taskResults.front() );
        _taskResults.pop_front();
        return result;
    }
    

    bool enqueueTaskRequest( const Ip6Addr& addr )
    {
        return _taskRequests.push( addr );
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
        return _schedulers[ addr ].enqueueTask( std::move( task ), completionType );
    }

    template< SerializableOrTrivial Result >
    bool enqueueTask( const Ip6Addr& addr, int functionId, FunctionCompletionType completionType, bool enqueueFront = false )
    {
        auto task = Task< Result >( ++_taskId, TaskStatus::Enqueued, functionId, enqueueFront );
        return _schedulers[ addr ].enqueueTask( std::make_unique< TaskBase >( task ), completionType );
    }

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    bool enqueueTask( const Ip6Addr& addr, int functionId, int priority, bool enqueueFront, FunctionCompletionType completionType, std::tuple< Arguments... >&& arguments )
    {
        auto task = Task< Result, Arguments... >( ++_taskId, TaskStatus::Enqueued, functionId, priority, enqueueFront, arguments );
        return _schedulers[ addr ].enqueueTask( std::make_unique< Task< Result, Arguments... > >( task ), completionType );
    }

    bool enqueueTask( const Ip6Addr& addr, const FunctionConcept& relatedFunction, const uint8_t* buffer )
    {
        auto task = relatedFunction.createTask();
        task->fillFromBuffer( buffer );
        updateTaskIdIfStale( task->id() );
        return _schedulers[ addr ].enqueueTask( std::move( task ), relatedFunction.completionType() );
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
            return std::nullopt;
        }
        
        return std::optional< std::reference_wrapper< TaskBase > >( *_initialTask );
    }

    std::optional< std::reference_wrapper< TaskBase > > popTask( const Ip6Addr& address, bool isLeader = false )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            return std::nullopt;
        }

        return taskQueue->second.popTask( isLeader );
    }

    void finishActiveTask( const Ip6Addr& address )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            return;
        }

        taskQueue->second.clearActiveTask();
    }

    void finishActiveTask( const Ip6Addr& address, int id )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            return;
        }

        taskQueue->second.clearActiveTask( id );
    }

    std::unique_ptr< TaskBase > finishAndGetActiveTask ( const Ip6Addr& address )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            return nullptr;
        }

        return taskQueue->second.clearAndGetActiveTask();
    }

    void unblockSchedulers( bool hardUnblock = false )
    {
        for( auto it = _schedulers.begin(); it != _schedulers.end(); ++it)
        {
            it->second.clearActiveTask( hardUnblock );
        }
    }

    void clearTasks()
    {
        unblockSchedulers();
        _schedulers.clear();
    }
};