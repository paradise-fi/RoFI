#pragma once

#include <algorithm>
#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskEntry.hpp"

class TaskScheduler
{
    std::vector< TaskEntry > _tasks;

    // TODO: This needs to be handled better -> we require manual clearing now, and that is not good at all.
    std::unique_ptr< TaskEntry > _active;

    void ageTasks()
    {
        for( auto taskEntry = _tasks.begin(); taskEntry != _tasks.end(); ++taskEntry)
        {
            taskEntry->task->incrementAge();
        }
    }

    void pushTaskToFront( std::unique_ptr< TaskBase > task, CompletionType completionType )
    {
        if ( !_tasks.empty() )
        {
            auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [](const TaskEntry& lhs, const TaskEntry& rhs ) { return lhs.task->getEffectivePriority() < rhs.task->getEffectivePriority(); } );

            task->setPriority( oldest->task->getEffectivePriority() + 1 );
        }

        _tasks.push_back( TaskEntry( std::move( task ), completionType ) );
    }

public:
    bool schedulerIsBlocked()
    {
        if ( _active == nullptr )
        {
            return false;
        }

        return _active.get()->completionType != CompletionType::NonBlocking;
    }

    void clearActiveTask()
    {
        if ( _active == nullptr )
        {
            return;
        }

        _active.reset( nullptr );
    }

    void clearActiveTask( int id )
    {
        if ( _active == nullptr )
        {
            return;
        }

        if ( _active.get()->task.get()->id() == id )
        {
            _active.reset();
        }
    }

    std::optional< std::reference_wrapper< TaskBase > > popTask( bool isLeader = false )
    {
        if ( _tasks.empty() || schedulerIsBlocked() )
        {
            return std::nullopt;
        }

        if ( isLeader )
        {
            ageTasks();
        }

        auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [](const TaskEntry& lhs, const TaskEntry& rhs ) { return lhs.task->getEffectivePriority() < rhs.task->getEffectivePriority(); } );

        std::swap( *oldest, _tasks.back() );
        auto taskEntry = std::move( _tasks.back() );
        _active.reset(nullptr);
        _active = std::make_unique< TaskEntry >( std::move( taskEntry.task ), taskEntry.completionType );
        _tasks.pop_back();

        return std::reference_wrapper< TaskBase >(*(_active.get()->task.get()));
    }

    void enqueueTask( std::unique_ptr< TaskBase > task, CompletionType completionType )
    {
        if ( task.get()->isQueuedToFront() )
        {
            pushTaskToFront( std::move( task ), completionType );
            return;
        }

        _tasks.push_back( TaskEntry( std::move( task ), completionType ) );
    }
};
