#include <queue>
#include "task.hpp"
#include "functionModel.hpp"

class TaskManager
{
    // This int should be atomic!
    int _taskId = 1;
    std::unique_ptr< TaskBase > _initialTask;
    std::map< Ip6Addr, std::queue< std::unique_ptr< TaskBase > > > _tasks;

    void updateTaskIdIfStale( int newId )
    {
        _taskId = _taskId < newId ? newId : _taskId;
    }

public:
    bool enqueueTask( const Ip6Addr& addr, std::unique_ptr< TaskBase >&&  task )
    {
        _tasks[ addr ].push( std::move( task ) );
        return true;
    }

    template< typename Result >
    bool enqueueTask( Ip6Addr addr, int functionId )
    {
        auto task = Task< Result >( ++_taskId, TaskStatus::Enqueued, functionId );
        _tasks[ addr ].push( std::make_unique< TaskBase >( task ) );

        return true;
    }

    template < typename Result, typename... Arguments >
    bool enqueueTask( Ip6Addr addr, int functionId, std::tuple< Arguments... >&& arguments )
    {
        auto task = Task< Result, Arguments... >( ++_taskId, TaskStatus::Enqueued, functionId, arguments );
        _tasks[ addr ].push( std::make_unique< Task< Result, Arguments... > >( task ) );

        return true;
    }

    bool enqueueTask( Ip6Addr addr, const FunctionConcept& relatedFunction, const PBuf& buffer, int index )
    {
        auto task = relatedFunction.createTask();
        task->fillFromBuffer( buffer, index );
        updateTaskIdIfStale( task->id() );
        _tasks[ addr ].push( std::move( task ) );
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

    std::optional< std::unique_ptr< TaskBase > > popTask( Ip6Addr& address )
    {
        const auto& taskQueue = _tasks.find( address );

        if ( taskQueue == _tasks.end() )
        {
            std::cout << "Address not found in tasks" << std::endl;
            return std::nullopt;
        }

        if ( taskQueue->second.empty() )
        {
            std::cout << "No task in queue." << std::endl;
            return std::nullopt;
        }

        auto task = std::move( taskQueue->second.back() );
        taskQueue->second.pop();
        return task;
    }
};