#include <algorithm>
#include <queue>
#include "task.hpp"
#include "functionModel.hpp"

class TaskScheduler
{
    std::vector< std::unique_ptr< TaskBase > > _tasks;

    void ageTasks()
    {
        for( auto task = _tasks.begin(); task != _tasks.end(); ++task)
        {
            task->get()->incrementAge();
        }
    }

public:
    std::optional< std::unique_ptr< TaskBase > > popTask( bool isLeader = false )
    {
        if ( _tasks.empty() )
        {
            return std::nullopt;
        }

        if ( isLeader )
        {
            ageTasks();
        }

        auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [](const std::unique_ptr< TaskBase >& lhs, const std::unique_ptr< TaskBase >& rhs ) { return lhs->getEffectivePriority() < rhs->getEffectivePriority(); } );

        std::swap( *oldest, _tasks.back() );
        auto task = std::move( _tasks.back() );
        _tasks.pop_back();

        return task;
    }

    void enqueueTask( std::unique_ptr< TaskBase > task )
    {
        _tasks.push_back( std::move( task ) );
    }

    void pushTaskToFront( std::unique_ptr< TaskBase > task )
    {
        if ( !_tasks.empty() )
        {
            auto oldest = std::max_element( _tasks.begin(), _tasks.end(), 
            [](const std::unique_ptr< TaskBase >& lhs, const std::unique_ptr< TaskBase >& rhs ) { return lhs->getEffectivePriority() < rhs->getEffectivePriority(); } );

            task->setPriority( oldest->get()->getEffectivePriority() + 1 );
        }

        _tasks.push_back( std::move( task ) );
    }
};
