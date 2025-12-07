#include "../../tasks/taskManager.hpp"

void TaskManager::registerBarrierFunction( int functionId )
{
    for( auto it = _schedulers.begin(); it != _schedulers.end(); ++it)
    {
        it->second.registerBarrier( functionId );
    }
}

bool TaskManager::enqueueTaskResult( std::unique_ptr< TaskBase > task, const Ip6Addr& origin, bool pushToFront )
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

std::optional< TaskResultEntry > TaskManager::popTaskResult()
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


bool TaskManager::enqueueTaskRequest( const Ip6Addr& addr )
{
    return _taskRequests.push( addr );
}

bool TaskManager::popTaskRequest( Ip6Addr& result )
{
    return _taskRequests.pop( result );
}

bool TaskManager::anyTaskRequests()
{
    return !_taskRequests.empty();
}

bool TaskManager::enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType )
{
    return _schedulers[ addr ].enqueueTask( std::move( task ), completionType );
}

bool TaskManager::enqueueTask( const Ip6Addr& addr, const FunctionConcept& relatedFunction, const uint8_t* buffer )
{
    auto task = relatedFunction.createTask();
    task->fillFromBuffer( buffer );
    updateTaskIdIfStale( task->id() );
    return _schedulers[ addr ].enqueueTask( std::move( task ), relatedFunction.completionType() );
}

bool TaskManager::setInitialTask( const FunctionConcept& relatedFunction )
{
    auto task = relatedFunction.createTask();
    _initialTask = std::move( task );

    return true;
}

std::optional< std::reference_wrapper< TaskBase > > TaskManager::getInitialTask()
{
    
    if ( _initialTask == nullptr )
    {
        return std::nullopt;
    }
    
    return std::optional< std::reference_wrapper< TaskBase > >( *_initialTask );
}

std::optional< std::reference_wrapper< TaskBase > > TaskManager::popTask( const Ip6Addr& address, bool isLeader )
{
    const auto& taskQueue = _schedulers.find( address );

    if ( taskQueue == _schedulers.end() )
    {
        return std::nullopt;
    }

    return taskQueue->second.popTask( isLeader );
}

void TaskManager::finishActiveTask( const Ip6Addr& address )
{
    const auto& taskQueue = _schedulers.find( address );

    if ( taskQueue == _schedulers.end() )
    {
        return;
    }

    taskQueue->second.clearActiveTask();
}

void TaskManager::finishActiveTask( const Ip6Addr& address, int id )
{
    const auto& taskQueue = _schedulers.find( address );

    if ( taskQueue == _schedulers.end() )
    {
        return;
    }

    taskQueue->second.clearActiveTask( id );
}

std::unique_ptr< TaskBase > TaskManager::finishAndGetActiveTask ( const Ip6Addr& address )
{
    const auto& taskQueue = _schedulers.find( address );

    if ( taskQueue == _schedulers.end() )
    {
        return nullptr;
    }

    return taskQueue->second.clearAndGetActiveTask();
}

void TaskManager::unblockSchedulers( bool hardUnblock )
{
    for( auto it = _schedulers.begin(); it != _schedulers.end(); ++it)
    {
        it->second.clearActiveTask( hardUnblock );
    }
}

void TaskManager::clearTasks()
{
    unblockSchedulers();
    _schedulers.clear();
}

// =================== PRIVATE
void TaskManager::updateTaskIdIfStale( int newId )
{
    _taskId = _taskId < newId ? newId : _taskId;
}