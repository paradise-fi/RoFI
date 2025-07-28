#pragma once

#include "taskManager.hpp"
#include <functional>

template< typename Result, typename... Arguments > 
class FunctionHandle
{
    int _functionId;
    CompletionType _completionType;
    TaskManager& _taskManager;

public:
    FunctionHandle( int functionId, CompletionType completionType, TaskManager& taskManager )
        : _functionId( functionId ), _completionType( completionType ), _taskManager( taskManager ) {}

    bool operator()( const Ip6Addr& receiver, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        auto result = _taskManager.enqueueTask< Result >(
            receiver, _functionId, priority,
            setTopPriority, _completionType, std::move( arguments ) );

        if ( result )
        {
            _taskManager.enqueueTaskRequest( receiver );
        }

        return result;
    }
};