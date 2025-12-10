#pragma once

#include "../src/tasks/taskManager.hpp"
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
    /// @param target The address of the participant who will execute the function.
    /// @param priority The priority of the task. Higher priority tasks take precedence.
    /// @param setTopPriority If true, this task will be given the highest priority.
    /// @param arguments The arguments to the task.
    /// @return True if the task was queued succesfully, otherwise false.
    bool operator()( const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        if ( !_taskManager.enqueueTask< Result >(
                target, _implementation.functionId(), priority,
                setTopPriority, _implementation.completionType(), 
                std::move( arguments ) ) )
        {
            return false;   
        }

        return _taskManager.enqueueTaskRequest( target );
    }
};