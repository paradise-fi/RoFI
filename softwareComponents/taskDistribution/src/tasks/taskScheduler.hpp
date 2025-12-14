#pragma once

#include <algorithm>
#include "task.hpp"
#include "../functions/functionModel.hpp"
#include "taskEntry.hpp"
#include <algorithm>

class TaskScheduler
{
    unsigned int _globalAge = 0;
    std::vector< TaskEntry > _tasks;
    std::vector< TaskEntry > _blockedTasks;

    std::optional< unsigned int > _activeBarrierTaskId;
    std::set< int > _registeredBarrierFunctionIds;

    std::unique_ptr< TaskEntry > _active;

    bool pushTaskToFront( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType, bool isRegisteredBarrier );

    void moveBlockedTasks();

    void normalizeTaskPriorities();

    void ageTasks();
public:
    TaskScheduler( std::set< int >& registeredBarrierFunctionIds );

    void registerBarrier( int barrierId );

    bool schedulerIsBlocked();

    void clearActiveTask( bool clearBarrier = false );

    void clearActiveTask( unsigned int id, bool clearBarrier = false );

    std::unique_ptr< TaskBase > clearAndGetActiveTask();

    std::optional< std::reference_wrapper< TaskBase > > popTask( bool isLeader = false );

    bool enqueueTask( std::unique_ptr< TaskBase > task, FunctionCompletionType completionType );
};
