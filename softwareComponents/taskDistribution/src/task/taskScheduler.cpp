#include "../../tasks/taskScheduler.hpp"

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
    }

    _active.reset( nullptr );
}

void TaskScheduler::clearActiveTask( int id, bool clearBarrier )
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

    auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
        [ this ]( const TaskEntry& lhs, const TaskEntry& rhs ) 
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

bool TaskScheduler::enqueueTask( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType )
{
    // This is where we figure out the task is a barrier and put it where it belongs.
    if ( _registeredBarrierFunctionIds.find( task->functionId() ) != _registeredBarrierFunctionIds.end() )
    {
        if ( _activeBarrierTaskId.has_value() )
        {
            std::cout << "ActiveBarrierTaskId Has Value!" << std::endl;
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

// ============ PRIVATE
void TaskScheduler::ageTasks()
{
    for( auto taskEntry = _tasks.begin(); taskEntry != _tasks.end(); ++taskEntry)
    {
        taskEntry->task->incrementAge();
    }
}

bool TaskScheduler::pushTaskToFront( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType )
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