#include <queue>
#include "task.hpp"
#include "functionModel.hpp"
#include "taskScheduler.hpp"

class TaskManager
{
    // ToDo: This int should be atomic!
    int _taskId = 1;
    std::unique_ptr< TaskBase > _initialTask;
//    std::map< Ip6Addr, std::queue< std::unique_ptr< TaskBase > > > _tasks;
    std::map< Ip6Addr, TaskScheduler > _schedulers;

    void updateTaskIdIfStale( int newId )
    {
        _taskId = _taskId < newId ? newId : _taskId;
    }

public:
    bool enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task )
    {
        _schedulers[ addr ].enqueueTask( std::move( task ) );
        // _tasks[ addr ].push( std::move( task ) );
        return true;
    }

    template< typename Result >
    bool enqueueTask( Ip6Addr addr, int functionId, bool enqueueFront = false )
    {
        auto task = Task< Result >( ++_taskId, TaskStatus::Enqueued, functionId, enqueueFront );
        _schedulers[ addr ].enqueueTask( std::make_unique< TaskBase >( task ) );

        return true;
    }

    template < typename Result, typename... Arguments >
    bool enqueueTask( Ip6Addr addr, int functionId, int priority, bool enqueueFront, std::tuple< Arguments... >&& arguments )
    {
        auto task = Task< Result, Arguments... >( ++_taskId, TaskStatus::Enqueued, functionId, priority, enqueueFront, arguments );
        _schedulers[ addr ].enqueueTask( std::make_unique< Task< Result, Arguments... > >( task ) );

        return true;
    }

    bool enqueueTask( Ip6Addr addr, const FunctionConcept& relatedFunction, const uint8_t* buffer )
    {
        auto task = relatedFunction.createTask();
        task->fillFromBuffer( buffer );
        updateTaskIdIfStale( task->id() );
        _schedulers[ addr ].enqueueTask( std::move( task ) );
        return true;
    }

    bool setInitialTask( const FunctionConcept& relatedFunction )
    {
        auto task = relatedFunction.createTask();
        _initialTask = std::move( task );

        return true;
    }

    std::optional< std::reference_wrapper< TaskBase > > getInitialTask()
    {
        
        if ( _initialTask == nullptr )
        {
            std::cout << "Initial task is null" << std::endl;
            return std::nullopt;
        }
        
        return std::optional< std::reference_wrapper< TaskBase > >( *_initialTask );
    }

    std::optional< std::unique_ptr< TaskBase > > popTask( Ip6Addr& address, bool isLeader = false )
    {
        const auto& taskQueue = _schedulers.find( address );

        if ( taskQueue == _schedulers.end() )
        {
            std::cout << "Address not found in tasks" << std::endl;
            return std::nullopt;
        }


        return taskQueue->second.popTask( isLeader );
    }

    void clearTasks()
    {
        _schedulers.clear();
    }
};