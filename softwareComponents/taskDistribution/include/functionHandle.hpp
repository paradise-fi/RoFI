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

    /// @brief Invokes the distributed function, registering it as a task into the distributed scheduler. Note that this is NOT thread safe. Modules should only queue tasks in a single thread.
    /// @param target The address of the participant who will execute the function.
    /// @param priority The priority of the task. Higher priority tasks take precedence. The largest priority you can set this way is 100.
    /// @param setTopPriority If true, this task will be given the highest priority. The priority parameter is ignored if this is set to true.
    /// @param arguments The arguments to the task.
    /// @return True if the task was queued succesfully, otherwise false.
    bool operator()( const Ip6Addr& target, unsigned int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        if ( priority > _taskManager.MaxUserAssignedPriority && !setTopPriority )
        {
            return false;
        }

        if ( !_taskManager.enqueueTask< Result >(
                target, _implementation.functionId(), priority,
                setTopPriority, _implementation.completionType(), 
                std::move( arguments ) ) )
        {
            return false;   
        }

        _taskManager.enqueueTaskRequest( target );
        return true;
    }
};