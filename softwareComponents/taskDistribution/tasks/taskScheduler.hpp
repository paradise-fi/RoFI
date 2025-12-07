#pragma once

#include <algorithm>
#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskEntry.hpp"

class TaskScheduler
{
    std::vector< TaskEntry > _tasks;
    
    std::optional< int > _activeBarrierTaskId;
    std::set< int > _registeredBarrierFunctionIds;

    std::unique_ptr< TaskEntry > _active;

    void ageTasks();

    bool pushTaskToFront( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType );

public:
    TaskScheduler( std::set< int >& registeredBarrierFunctionIds );

    void registerBarrier( int barrierId );

    bool schedulerIsBlocked();

    void clearActiveTask( bool clearBarrier = false );

    void clearActiveTask( int id, bool clearBarrier = false );

    std::unique_ptr< TaskBase > clearAndGetActiveTask();

    std::optional< std::reference_wrapper< TaskBase > > popTask( bool isLeader = false );

    bool enqueueTask( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType );
};
