#pragma once

#include "../functions/functionManager.hpp"
#include "../tasks/taskManager.hpp"
#include "lwip++.hpp"

using namespace rofi::net;

class FunctionRegistry
{
    FunctionManager _functionManager;
    TaskManager _taskManager;

public:   
    template < typename Result, typename... Arguments >
    bool registerBarrier( std::unique_ptr< DistributedFunction< Result, Arguments... > > barrierFunction )
    {
        if ( barrierFunction->completionType() != FunctionCompletionType::Blocking )
        {
            return false;
        }

        int functionId = barrierFunction->functionId();

        if (!_functionManager.addFunction< Result, Arguments... >( std::move( barrierFunction ) ) )
        {
            return false;
        }

        _taskManager.registerBarrierFunction( functionId );
        return true; 
    }

    template < typename Result, typename... Arguments >
    bool registerFunction( std::unique_ptr< DistributedFunction< Result, Arguments... > > userFunction )
    {
        return _functionManager.addFunction< Result, Arguments... >( std::move( userFunction ) );
    }

    bool unregisterFunction( int id )
    {
        return _functionManager.removeFunction( id );
    }

    template< typename Result, typename... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( int functionId )
    {
        auto fn = _functionManager.getFunction( functionId );

        if ( !fn.has_value() )
        {
            return std::nullopt;
        }

        auto& model = static_cast< FunctionModel< Result, Arguments... >& >( fn->get() );

        return FunctionHandle< Result, Arguments... >( functionId, fn->get().completionType(), model.getImplementation(), _taskManager );
    }

    template< typename Result, typename... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( std::string functionName )
    {
        auto fn = _functionManager.getFunction( functionName );

        if ( !fn.has_value() )
        {
            return std::nullopt;
        }

        return FunctionHandle< Result, Arguments... >( fn.value().get().functionId(), fn.value().get().completionType(),  _taskManager );
    }

    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( int functionId )
    {
        return _functionManager.getFunction( functionId );
    }

    bool invokeFunction( TaskBase& task )
    {
        return _functionManager.invokeFunction( task );
    }

    bool invokeFunctionReaction( Ip6Addr& sender, TaskBase& task )
    {
        return _functionManager.invokeReaction( sender, task );
    }

    /*
    * Takes existing functionId and creates a task for it that is given at initialization to every requesting module.
    */
    bool setInitialTask( int functionId )
    {
        auto fn = _functionManager.getFunction( functionId );
        if ( !fn.has_value() )
        {
            return false;
        }
        
        return _taskManager.setInitialTask( fn.value() );
    }

    void enqueueTaskRequest( Ip6Addr& sender )
    {
        _taskManager.enqueueTaskRequest( sender );
    }

    void enqueueTask( Ip6Addr& address, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType )
    {
        _taskManager.enqueueTask( address, std::move( task ), completionType );
    }

    std::optional< Ip6Addr > getTaskRequester()
    {
        Ip6Addr requester( 1 );

        if ( !_taskManager.popTaskRequest( requester ) )
        {
            return std::nullopt;
        }

        return requester;
    }

    std::optional< std::reference_wrapper< TaskBase > > popTaskForAddress( Ip6Addr& address, bool isLeader = false )
    {
        auto task = _taskManager.popTask( address, isLeader );
        if ( !task.has_value() && isLeader )
        {
            task = _taskManager.getInitialTask();
        }
        return task;
    }

    void finishActiveTask( Ip6Addr& address )
    {
        _taskManager.finishActiveTask( address );
    }

    bool anyTaskRequests()
    {
        return _taskManager.anyTaskRequests();
    }

    std::unique_ptr< TaskBase > getTaskFromBuffer( uint8_t* buffer, int functionId )
    {
        auto fnOptional = _functionManager.getFunction( functionId );

        if ( !fnOptional.has_value() )
        {
            // Something is wrong!
            return nullptr;
        }

        auto fun = fnOptional.value();
        auto task = fun.get().createTask();
        task->fillFromBuffer( buffer );
        return task;
    }

    void clearTasks()
    {
        _taskManager.clearTasks();
    }

    void unblockTaskSchedulers()
    {
        _taskManager.unblockSchedulers();
    }
};