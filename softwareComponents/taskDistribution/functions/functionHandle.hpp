#pragma once

#include "taskManager.hpp"
#include <functional>

template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments > 
class FunctionHandle
{
    int _functionId;
    FunctionCompletionType _completionType;
    DistributedFunction< Result, Arguments... >& _implementation;
    TaskManager& _taskManager;

public:
    FunctionHandle( 
        int functionId, 
        FunctionCompletionType completionType,
        DistributedFunction< Result, Arguments... >& implementation,
        TaskManager& taskManager )
        : _functionId( functionId ),
          _completionType( completionType ),
          _implementation( implementation ),
          _taskManager( taskManager ) {}

    DistributedFunction< Result, Arguments...>& implementation()
    {
        return _implementation;
    }

    /// @brief Invokes the distributed function, registering it as a task into the distributed scheduler.
    /// @param receiver The address of the participant who will execute the function.
    /// @param priority The priority of the task. Higher priority tasks take precedence.
    /// @param setTopPriority If true, this task will be given the highest priority.
    /// @param arguments The arguments to the task.
    /// @return True if the task was queued succesfully, otherwise false.
    bool operator()( const Ip6Addr& receiver, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        auto result = _taskManager.enqueueTask< Result >(
            receiver, _implementation.functionId(), priority,
            setTopPriority, _implementation.completionType(), std::move( arguments ) );

        if ( result )
        {
            result = _taskManager.enqueueTaskRequest( receiver );
        }

        return result;
    }
};