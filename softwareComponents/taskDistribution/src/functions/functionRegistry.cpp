#include "functionRegistry.hpp"

FunctionRegistry::FunctionRegistry( LoggingService& loggingService ) : _loggingService( loggingService ), _functionManager( loggingService ) {}

bool FunctionRegistry::unregisterFunction( int id )
{
    return _functionManager.removeFunction( id );
}

/// @brief Retrieves an internal function object. May be useful in some scenarios, but this is mostly used for internal worfklows.
/// @param functionId The ID of the function.
/// @return std::nullopt if the function does not exist, otherwise it returns a type-erased FunctionConcept.
std::optional< std::reference_wrapper< FunctionConcept > > FunctionRegistry::getFunction( int functionId )
{
    return _functionManager.getFunction( functionId );
}

/// @brief Invokes a function represented by task.
/// @param task The task representing the function to be invoked.
/// @return True if the function invocation went well.
bool FunctionRegistry::invokeFunction( TaskBase& task )
{
    return _functionManager.invokeFunction( task );
}

/// @brief Invokes a reaction portion of the function repsented by task.
/// @param sender The address of the module sending the result.
/// @param task The task representing the function to be reacted to.
/// @return The result of the function contained in the FunctionResultType enum.
FunctionResultType FunctionRegistry::invokeFunctionReaction( const Ip6Addr& sender, TaskBase& task )
{
    return _functionManager.invokeReaction( sender, task );
}

bool FunctionRegistry::processTaskResult( std::unique_ptr< TaskBase > task, const Ip6Addr& sender )
{
    std::cout << "processTaskResult" << std::endl;
    if ( task == nullptr )
    {
        _loggingService.logError( "processTaskResult - Task in TaskResult is null." );
        return false;
    }

    if ( task->status() == TaskStatus::RepeatDistributed )
    {
        auto fn = _functionManager.getFunction( task->functionId() );
        if ( !fn.has_value() )
        {
            _loggingService.logError("Trying to requeue task in result processing, but no such function exists.");
        }

        return _taskManager.enqueueTask( sender, std::move( task ), fn.value().get().completionType() );
    }

    auto reactionResult = invokeFunctionReaction( sender, *task.get() );

    if ( reactionResult == FunctionResultType::FAILURE )
    {
        std::ostringstream stream;
        stream << "processTaskResultQueue - Function reaction invocation failed for Task " << task->id();
        _loggingService.logError( stream.str() );
        return false;
    }

    if ( reactionResult == FunctionResultType::TRY_AGAIN_LOCAL )
    {
        
        _loggingService.logInfo( "processTaskResultQueue - Task result placed back in queue.", LogVerbosity::High );
        if ( !_taskManager.enqueueTaskResult( std::move( task ), sender, true ) )
        {
            _loggingService.logError( "processTaskResultQueue - Failed to enqueue task result." );
            return false;
        }
    }

    return true;
}

/// @brief Places a module's request into the task request queue.
/// @param requester The address of the task requester
/// @return True if the task request was placed in the queue.
bool FunctionRegistry::enqueueTaskRequest( const Ip6Addr& requester )
{
    return _taskManager.enqueueTaskRequest( requester );
}

/// @brief Places a task into the module's task queue.
/// @param address The address of the task executor and their queue.
/// @param task The task.
/// @param completionType The function completion type - blocking, non-blocking.
/// @return True if the task was enqueued succesfully.
bool FunctionRegistry::enqueueTask( const Ip6Addr& address, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType )
{
    return _taskManager.enqueueTask( address, std::move( task ), completionType );
}

/// @brief Retrieves a task requester from the task request queue.
/// @return std::nullopt if there is no requester. Otherwise, the requester address.
std::optional< Ip6Addr > FunctionRegistry::getTaskRequester()
{
    Ip6Addr requester( 1 );

    if ( !_taskManager.popTaskRequest( requester ) )
    {
        return std::nullopt;
    }

    return requester;
}

std::optional< std::reference_wrapper< TaskBase > > FunctionRegistry::popTaskForAddress( const Ip6Addr& address, bool isLeader )
{
    auto task = _taskManager.popTask( address, isLeader );
    if ( !task.has_value() && isLeader )
    {
        task = _taskManager.getInitialTask();
    }
    return task;
}

/// @brief Clears the active task in a queue representing the module whose address is passed to this function. Used for internal functionality.
/// @param address The address of the module.
void FunctionRegistry::finishActiveTask( const Ip6Addr& address )
{
    _taskManager.finishActiveTask( address );
}

/// @brief Clears the active task in a queue representing the module whose address is passed to this function. Then returns the active task. Used for internal functionality.
/// @param address The address of the module.
/// @return The active task if there is any.
std::unique_ptr< TaskBase > FunctionRegistry::finishAndGetActiveTask ( const Ip6Addr& address )
{
    return _taskManager.finishAndGetActiveTask( address );
}

/// @brief Checks if there are any task requests in the task request queue.
/// @return True if there are requests, else false.
bool FunctionRegistry::anyTaskRequests()
{
    return _taskManager.anyTaskRequests();
}

/// @brief Parses a buffer into a task that represents the function with functionId
/// @param buffer The buffer that will be parsed
/// @param functionId The ID of the function that the task will represent
/// @return The task
std::unique_ptr< TaskBase > FunctionRegistry::getTaskFromBuffer( uint8_t* buffer, int functionId )
{
    auto fnOptional = _functionManager.getFunction( functionId );

    if ( !fnOptional.has_value() )
    {
        // Something is wrong!
        return nullptr;
    }

    auto fun = fnOptional.value();
    auto task = fun.get().createTask();
    task->fillFromBuffer( buffer );
    return task;
}

/// @brief Removes all tasks from all schedulers on this module.
void FunctionRegistry::clearTasks()
{
    _taskManager.clearTasks();
}

/// @brief Clears all task schedulers for scheduling tasks.
/// @param hardUnblock Removes active barrier if true, otherwise the barrier remains active.
void FunctionRegistry::unblockTaskSchedulers( bool hardUnblock )
{
    _taskManager.unblockSchedulers( hardUnblock );
}

void FunctionRegistry::unblockTaskScheduler( const Ip6Addr& schedulerAddr, bool hardUnblock )
{
    _taskManager.unblockScheduler( schedulerAddr, hardUnblock );
}
