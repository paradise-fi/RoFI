#pragma once

#include <algorithm>
#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskEntry.hpp"

class TaskScheduler
{
    std::vector< TaskEntry > _tasks;
    
    std::optional< int > _registeredBarrierFunctionId;
    std::optional< int > _activeBarrierTaskId;

    // TODO: This needs to be handled better -> we require manual clearing now, and that is not good at all.
    std::unique_ptr< TaskEntry > _active;

    void ageTasks()
    {
        for( auto taskEntry = _tasks.begin(); taskEntry != _tasks.end(); ++taskEntry)
        {
            taskEntry->task->incrementAge();
        }
    }

    bool pushTaskToFront( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType )
    {
        if ( !_tasks.empty() )
        {
            auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [](const TaskEntry& lhs, const TaskEntry& rhs ) { return lhs.task->getEffectivePriority() < rhs.task->getEffectivePriority(); } );

            task->setPriority( oldest->task->getEffectivePriority() + 1 );
        }

        _tasks.push_back( TaskEntry( std::move( task ), completionType ) );
        return true;
    }

public:
    void registerBarrier( int barrierId )
    {
        std::cout << "Barrier registered as " << barrierId << std::endl;
        _registeredBarrierFunctionId = barrierId;
    }

    bool schedulerIsBlocked()
    {
        if ( _active == nullptr )
        {
            return false;
        }

        return _active.get()->completionType != FunctionCompletionType::NonBlocking;
    }

    void clearActiveTask( bool clearBarrier = false )
    {
        if ( _active == nullptr )
        {
            return;
        }

        if ( _active->task->id() == _activeBarrierTaskId)
        {
            std::cout << "Clearing barrier" << std::endl;
            if ( !clearBarrier )
            {
                std::cout << "ACTUALLY NOT DOING SO" << std::endl;
                return;
            }

            _activeBarrierTaskId = std::nullopt;
        }

        _active.reset( nullptr );
    }

    void clearActiveTask( int id, bool clearBarrier = false )
    {
        if ( _active == nullptr )
        {
            return;
        }

        if ( _active.get()->task.get()->id() == id )
        {
            if ( _active->task->id() == _activeBarrierTaskId )
            {
                std::cout << "Clearing barrier" << std::endl;
                if ( !clearBarrier )
                {
                    std::cout << "ACTUALLY NOT DOING SO" << std::endl;
                    return;
                }

                _activeBarrierTaskId = std::nullopt;
            }
            
            _active.reset();
        }
    }

    std::unique_ptr< TaskBase > clearAndGetActiveTask()
    {
        if ( _active == nullptr )
        {
            return nullptr;
        }

        auto activeTask = std::move( _active->task );
        _active = nullptr;
        return activeTask;
    }

    std::optional< std::reference_wrapper< TaskBase > > popTask( bool isLeader = false )
    {
        if ( ( !isLeader && schedulerIsBlocked() ) )
        {
            std::cout << "Blocked." << std::endl;
        }

        if ( _tasks.empty() || ( !isLeader && schedulerIsBlocked() ) )
        {
            return std::nullopt;
        }

        if ( isLeader )
        {
            ageTasks();
        }

        if ( _activeBarrierTaskId.has_value() )
        {
            std::cout << "Active Barrier Task ID: " << _activeBarrierTaskId.value() << std::endl;
        }

        auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [this](const TaskEntry& lhs, const TaskEntry& rhs ) 
            { 
                if ( _activeBarrierTaskId.has_value() && rhs.task->id() > _activeBarrierTaskId.value() )
                {
                    return false;
                }

                return lhs.task->getEffectivePriority() < rhs.task->getEffectivePriority(); 
            } );

        std::swap( *oldest, _tasks.back() );
        auto taskEntry = std::move( _tasks.back() );
        
        _active.reset(nullptr);
        _active = std::make_unique< TaskEntry >( std::move( taskEntry.task ), taskEntry.completionType );

        _tasks.pop_back();

        return std::reference_wrapper< TaskBase >(*(_active.get()->task.get()));
    }

    bool enqueueTask( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType )
    {
        // This is where we figure out the task is a barrier and put it where it belongs.
        if ( task->functionId() == _registeredBarrierFunctionId )
        {
            if ( _activeBarrierTaskId.has_value() )
            {
                std::cout << "Task not queued, barrier is already set." << std::endl;
                return false;
            }

            _activeBarrierTaskId = task->id();
        }

        // We have to make sure that a task being queued when a barrier is active does not queue over it.
        if ( task.get()->isQueuedToFront() )
        {
            return pushTaskToFront( std::move( task ), completionType );
        }

        _tasks.push_back( TaskEntry( std::move( task ), completionType ) );
        return true;
    }
};
