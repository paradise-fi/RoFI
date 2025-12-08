#pragma once

#include "functionManager.hpp"
#include "../tasks/taskManager.hpp"
#include "lwip++.hpp"
#include "../../include/serializable.hpp"
#include "../logger/loggingService.hpp"

using namespace rofi::net;

class FunctionRegistry
{
    LoggingService& _loggingService;
    FunctionManager _functionManager;
    TaskManager _taskManager;

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
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

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    bool registerInitialFunction( std::unique_ptr< DistributedFunction< Result, Arguments... > > initialFunction  )
    {
        int functionId = initialFunction->functionId();

        if ( !_functionManager.addFunction< Result, Arguments... >( std::move( initialFunction ) ) )
        {
            return false;
        }

        auto fn = _functionManager.getFunction( functionId );

        if ( !fn.has_value() )
        {
            return false;
        }

        return _taskManager.setInitialTask( fn.value() );
    }

public:   
    FunctionRegistry( LoggingService& loggingService );

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments,
              std::derived_from< DistributedFunction< Result, Arguments... > > Func >
    bool registerFunction( const Func& function )
    {
        std::unique_ptr< DistributedFunction< Result, Arguments... > > userFunction = std::make_unique< Func >( function );
        if ( userFunction->functionType() == FunctionType::Barrier )
        {
            return registerBarrier( std::move( userFunction ) );
        }

        if ( userFunction->functionType() == FunctionType::Initial )
        {
            return registerInitialFunction( std::move( userFunction ) );
        }

        return _functionManager.addFunction< Result, Arguments... >( std::move( userFunction ) );
    }

    bool unregisterFunction( int id );

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionId The ID of the distributed function. You may also use the std::string variant for more human-friendly retrieval.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
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

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionName The name of the distributed function.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( const std::string& functionName )
    {
        auto fn = _functionManager.getFunction( functionName );

        if ( !fn.has_value() )
        {
            return std::nullopt;
        }

        auto& model = static_cast< FunctionModel< Result, Arguments... >& >( fn->get() );

        return FunctionHandle< Result, Arguments... >( fn.value().get().functionId(), fn.value().get().completionType(), model.getImplementation(), _taskManager );
    }

    /// @brief Retrieves an internal function object. May be useful in some scenarios, but this is mostly used for internal worfklows.
    /// @param functionId The ID of the function.
    /// @return std::nullopt if the function does not exist, otherwise it returns a type-erased FunctionConcept.
    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( int functionId );

    /// @brief Invokes a function represented by task.
    /// @param task The task representing the function to be invoked.
    /// @return True if the function invocation went well.
    bool invokeFunction( TaskBase& task );

    /// @brief Invokes a reaction portion of the function repsented by task.
    /// @param sender The address of the module sending the result.
    /// @param task The task representing the function to be reacted to.
    /// @return The result of the function contained in the FunctionResultType enum.
    FunctionResultType invokeFunctionReaction( Ip6Addr& sender, TaskBase& task );

    /// @brief Places a task into the sender's queue.
    /// @param task The task
    /// @param sender The address of the queue
    /// @return True if the enqueue operation succeeded.
    bool enqueueTaskResult( std::unique_ptr< TaskBase > task, Ip6Addr sender );

    /// @brief Handles the flow of the task result queue.
    void processTaskResultQueue();

    /// @brief Places a module's request into the task request queue.
    /// @param requester The address of the task requester
    /// @return True if the task request was placed in the queue.
    bool enqueueTaskRequest( const Ip6Addr& requester );

    /// @brief Places a task into the module's task queue.
    /// @param address The address of the task executor and their queue.
    /// @param task The task.
    /// @param completionType The function completion type - blocking, non-blocking.
    /// @return True if the task was enqueued succesfully.
    bool enqueueTask( const Ip6Addr& address, std::unique_ptr< TaskBase >&&  task, FunctionCompletionType completionType );

    /// @brief Retrieves a task requester from the task request queue.
    /// @return std::nullopt if there is no requester. Otherwise, the requester address.
    std::optional< Ip6Addr > getTaskRequester();

    std::optional< std::reference_wrapper< TaskBase > > popTaskForAddress( const Ip6Addr& address, bool isLeader = false );

    /// @brief Clears the active task in a queue representing the module whose address is passed to this function. Used for internal functionality.
    /// @param address The address of the module.
    void finishActiveTask( const Ip6Addr& address );

    /// @brief Clears the active task in a queue representing the module whose address is passed to this function. Then returns the active task. Used for internal functionality.
    /// @param address The address of the module.
    /// @return The active task if there is any.
    std::unique_ptr< TaskBase > finishAndGetActiveTask ( const Ip6Addr& address );

    /// @brief Checks if there are any task requests in the task request queue.
    /// @return True if there are requests, else false.
    bool anyTaskRequests();

    /// @brief Parses a buffer into a task that represents the function with functionId
    /// @param buffer The buffer that will be parsed
    /// @param functionId The ID of the function that the task will represent
    /// @return The task
    std::unique_ptr< TaskBase > getTaskFromBuffer( uint8_t* buffer, int functionId );

    /// @brief Removes all tasks from all schedulers on this module.
    void clearTasks();

    /// @brief Clears all task schedulers for scheduling tasks.
    /// @param hardUnblock Removes active barrier if true, otherwise the barrier remains active and only blocking regular function tasks are removed.
    void unblockTaskSchedulers( bool hardUnblock = false );

    /// @brief Clears specific task scheduler on this module.
    /// @param schedulerAddr - The address of the schedulder
    /// @param hardUnblock Removes active barrier if true, otherwise the barrier remains active and only blocking regular function tasks are removed.
    void unblockTaskScheduler( const Ip6Addr& schedulerAddr, bool hardUnblock = false );

};