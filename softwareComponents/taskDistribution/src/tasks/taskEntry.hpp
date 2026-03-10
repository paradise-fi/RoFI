#pragma once

#include <memory>

#include "task.hpp"
#include "../functions/enums/functionCompletionType.hpp"

struct TaskEntry
{
    TaskEntry( std::unique_ptr< TaskBase >&& task, FunctionCompletionType completionType, unsigned int onPushGlobalAge )
    : task( std::move( task ) ), completionType( completionType ), onPushGlobalAge( onPushGlobalAge ) {}

    std::unique_ptr< TaskBase > task;
    FunctionCompletionType completionType;
    unsigned int onPushGlobalAge = 0;

    unsigned int priorityTimestamp( unsigned int globalAge )
    {
        return globalAge - onPushGlobalAge;
    }

    unsigned int effectivePriority( unsigned int globalAge )
    {
        return task->getPriority() + priorityTimestamp( globalAge );
    }
};

struct TaskEntryComparator
{
    unsigned int globalAge;

    TaskEntryComparator( unsigned int globalAge ) : globalAge( globalAge ) {}

    bool operator()( const TaskEntry& lhs, const TaskEntry& rhs ) const {
        if ( lhs.task == nullptr && rhs.task == nullptr )
        {
            return false;
        }

        if ( lhs.task == nullptr )
        {
            return true;
        }

        if ( rhs.task == nullptr )
        {
            return false;
        }

        return lhs.task->getPriority() + ( globalAge - lhs.onPushGlobalAge ) < 
               rhs.task->getPriority() + ( globalAge - rhs.onPushGlobalAge );
    }
};