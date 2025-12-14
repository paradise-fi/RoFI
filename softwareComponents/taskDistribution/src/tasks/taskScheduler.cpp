#include "taskScheduler.hpp"

TaskScheduler::TaskScheduler( std::set< int >& registeredBarrierFunctionIds ) : _registeredBarrierFunctionIds( registeredBarrierFunctionIds ) {}

void TaskScheduler::registerBarrier( int barrierId )
{
    _registeredBarrierFunctionIds.emplace( barrierId );
}

bool TaskScheduler::schedulerIsBlocked()
{
    if ( _active == nullptr || _active.get() == nullptr )
    {
        return false;
    }

    return _active.get()->completionType != FunctionCompletionType::NonBlocking;
}

void TaskScheduler::clearActiveTask( bool clearBarrier )
{
    if ( _active == nullptr || _active.get() == nullptr )
    {
        return;
    }

    if ( _active->task->id() == _activeBarrierTaskId)
    {
        if ( !clearBarrier )
        {
            return;
        }

        _activeBarrierTaskId = std::nullopt;
        moveBlockedTasks();
    }

    _active.reset( nullptr );
}

void TaskScheduler::clearActiveTask( unsigned int id, bool clearBarrier )
{
    if ( _active == nullptr || _active.get() == nullptr )
    {
        return;
    }

    if ( _active.get()->task == nullptr || _active.get()->task.get() == nullptr )
    {
        _active.reset();
        return;
    }

    if ( _active.get()->task.get()->id() == id )
    {
        if ( _active->task->id() == _activeBarrierTaskId )
        {
            if ( !clearBarrier )
            {
                return;
            }

            _activeBarrierTaskId = std::nullopt;
            moveBlockedTasks();
        }
        
        _active.reset();
    }
}

std::unique_ptr< TaskBase > TaskScheduler::clearAndGetActiveTask()
{
    if ( _active == nullptr || _active.get() == nullptr )
    {
        return nullptr;
    }

    if ( _active.get()->task == nullptr || _active.get()->task.get() == nullptr )
    {
        _active.reset();
        return nullptr;
    }

    if ( _active->task->id() == _activeBarrierTaskId )
    {
        _activeBarrierTaskId = std::nullopt;
        moveBlockedTasks();
    }

    auto activeTask = std::move( _active->task );
    _active = nullptr;
    return activeTask;
}

std::optional< std::reference_wrapper< TaskBase > > TaskScheduler::popTask( bool isLeader )
{
    if ( _tasks.empty() || ( isLeader && schedulerIsBlocked() ) )
    {
        return std::nullopt;
    }

    if ( isLeader )
    {
        ageTasks();
    }

    std::pop_heap( _tasks.begin(), _tasks.end(), TaskEntryComparator( _globalAge ) );
    auto taskEntry = std::move( _tasks.back() );

    _tasks.pop_back();

    _active.reset(nullptr);
    unsigned int effectivePriority = taskEntry.effectivePriority( _globalAge );
    _active = std::make_unique< TaskEntry >( std::move( taskEntry.task ), taskEntry.completionType, taskEntry.onPushGlobalAge );
    
    // Update priority to match the final result.
    _active->task->setPriority( effectivePriority );

    return std::reference_wrapper< TaskBase >(*(_active.get()->task.get()));
}

bool TaskScheduler::enqueueTask( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType )
{
    // This is where we figure out the task is a barrier and put it where it belongs.
    bool isRegisteredBarrier = _registeredBarrierFunctionIds.find( task->functionId() ) != _registeredBarrierFunctionIds.end();
    if ( isRegisteredBarrier )
    {
        if ( _activeBarrierTaskId.has_value() )
        {
            return false;
        }

        _activeBarrierTaskId = task->id();
    }

    // We have to make sure that a task being queued when a barrier is active does not queue over it.
    if ( task.get()->isQueuedToFront() )
    {
        return pushTaskToFront( std::move( task ), completionType, isRegisteredBarrier );
    }

    if ( _activeBarrierTaskId.has_value() && !isRegisteredBarrier )
    {
        _blockedTasks.push_back( TaskEntry( std::move( task ), completionType, _globalAge ) );
        std::push_heap( _blockedTasks.begin(), _blockedTasks.end(), TaskEntryComparator( _globalAge ) );
        return true;
    }

    _tasks.push_back( TaskEntry( std::move( task ), completionType, _globalAge ) );
    std::push_heap(_tasks.begin(), _tasks.end(), TaskEntryComparator( _globalAge ) );
    return true;
}

void TaskScheduler::ageTasks()
{
    _globalAge++;
    // Magical constants - todo
    if ( _globalAge + 1000 > std::numeric_limits< unsigned int >::max() - 5000 )
    {
        normalizeTaskPriorities();
    }
}

// ============ PRIVATE

bool TaskScheduler::pushTaskToFront( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType, bool isRegisteredBarrier )
{
    if ( !_tasks.empty() )
    {
        task->setPriority( _tasks.front().effectivePriority( _globalAge ) );
    }
    else if ( !_blockedTasks.empty() )
    {
        task->setPriority( _blockedTasks.front().effectivePriority( _globalAge ) );
    }


    if ( _activeBarrierTaskId.has_value() && !isRegisteredBarrier )
    {
        _blockedTasks.push_back( TaskEntry( std::move( task ), completionType, _globalAge ) );
        std::push_heap( _blockedTasks.begin(), _blockedTasks.end(), TaskEntryComparator( _globalAge ) );
        return true;
    }

    _tasks.push_back( TaskEntry( std::move( task ), completionType, _globalAge ) );
    std::push_heap( _tasks.begin(), _tasks.end(), TaskEntryComparator( _globalAge ) );
    return true;
}

void TaskScheduler::moveBlockedTasks()
{
    _tasks.reserve( _tasks.size() + _blockedTasks.size() );
    _tasks.insert( _tasks.end(), std::make_move_iterator( _blockedTasks.begin() ), std::make_move_iterator( _blockedTasks.end() ) );
    _blockedTasks.clear();
    std::make_heap( _tasks.begin(), _tasks.end(), TaskEntryComparator( _globalAge ) );
}

void TaskScheduler::normalizeTaskPriorities()
{
    for ( auto it = _tasks.begin(); it != _tasks.end(); ++it )
    {
        it->task->addPriority( it->priorityTimestamp( _globalAge ) );
    }

    for ( auto ibt = _blockedTasks.begin(); ibt != _blockedTasks.end(); ++ibt )
    {
        ibt->task->addPriority( ibt->priorityTimestamp( _globalAge ) );
    }

    _globalAge = 0;

    std::make_heap( _tasks.begin(), _tasks.end(), TaskEntryComparator( 0 ) );
    std::make_heap( _blockedTasks.begin(), _blockedTasks.end(), TaskEntryComparator( 0 ) );
}