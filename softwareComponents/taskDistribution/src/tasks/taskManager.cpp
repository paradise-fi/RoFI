#include "taskManager.hpp"

void TaskManager::registerBarrierFunction( int functionId )
{
    _barrierFunctions.emplace( functionId );
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
    auto success = _taskRequests.pop( result );
    if ( !success )
    {
        return false;
    }

    auto scheduler = _schedulers.find( result );

    if ( scheduler == _schedulers.end() )
    {
        return true;
    }

    if ( scheduler->second.schedulerIsBlocked() )
    {
        _taskRequests.push( result );
        return false;
    }

    return true;
}

bool TaskManager::anyTaskRequests()
{
    return !_taskRequests.empty();
}

bool TaskManager::enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType )
{
    updateTaskIdIfStale( task->id() );

    return enqueueTaskInternal( addr, std::move( task ), completionType );
}

bool TaskManager::enqueueTask( const Ip6Addr& addr, const FunctionConcept& relatedFunction, const uint8_t* buffer )
{
    auto task = relatedFunction.createTask();
    task->fillFromBuffer( buffer );
    updateTaskIdIfStale( task->id() );

    return enqueueTaskInternal( addr, std::move( task ), relatedFunction.completionType() );
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

void TaskManager::finishActiveTask( const Ip6Addr& address, unsigned int id )
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

void TaskManager::unblockScheduler( const Ip6Addr& address, bool hardUnblock )
{
    auto scheduler = _schedulers.find( address );

    if ( scheduler == _schedulers.end() )
    {
        return;
    }
    
    scheduler->second.clearActiveTask( hardUnblock );
}

void TaskManager::clearTasks()
{
    unblockSchedulers();
    _schedulers.clear();
}

// =================== PRIVATE
void TaskManager::updateTaskIdIfStale( unsigned int newId )
{
    // Overflow reset - todo magic constant
    if ( _taskId > newId && _taskId - newId > std::numeric_limits< unsigned int >::max() - 5000 )
    {
        _taskId = newId;
    }

    _taskId = _taskId < newId ? newId : _taskId;
}

bool TaskManager::enqueueTaskInternal( const Ip6Addr& addr, std::unique_ptr< TaskBase >&& task, FunctionCompletionType completionType )
{
    auto scheduler = _schedulers.find( addr );
    if ( scheduler != _schedulers.end() )
    {
        return scheduler->second.enqueueTask( std::move( task ), completionType );
    }

    auto emplacedScheduler = _schedulers.emplace( addr, TaskScheduler( _barrierFunctions ) );
    if ( !emplacedScheduler.second )
    {
        return false;
    }

    return emplacedScheduler.first->second.enqueueTask( std::move( task ), completionType );
}