#pragma once

#include "taskManager.hpp"
#include <functional>

template< typename Result, typename... Arguments > 
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

    bool operator()( const Ip6Addr& receiver, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        std::cout << "Going to enqueue task in FunctionHandle." << std::endl;
        auto result = _taskManager.enqueueTask< Result >(
            receiver, _functionId, priority,
            setTopPriority, _completionType, std::move( arguments ) );


        std::cout << "After enqueue task in FunctionHandle" << std::endl;
        if ( result )
        {
            _taskManager.enqueueTaskRequest( receiver );
        }

        return result;
    }
};