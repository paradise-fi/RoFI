#pragma once

#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "naiveBarrier.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class InitialFunction : public DistributedFunction< int >
{
    int _moduleId;
    DistributedTaskManager& _manager;

public:
    InitialFunction( int moduleId, DistributedTaskManager& manager )
    : _moduleId( moduleId ),
      _manager( manager ) {}

    virtual FunctionResult< int > execute() override
    {
        return FunctionResult< int >( _moduleId, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            std::cout << "No result value" << std::endl;
            return false;
        }

        // IMPORTANT!!! - Register participants into the naive barrier.
        auto barrierHandle = _manager.functions().getFunctionHandle< Ip6Addr >( "NaiveBarrier" ).value();
        auto& barrierImplementation = static_cast< NaiveBarrier& >( barrierHandle.implementation() );
        barrierImplementation.registerParticipant( origin );

        int moduleId = result.value();

        std::cout << "Initial ModuleId: " << moduleId << std::endl;

        if ( moduleId > 2 )
        {
            std::cout << "This module will not execute any other distributed functions." << std::endl;
            return false;
        }

        auto blockingHandle = _manager.functions().getFunctionHandle< int, int >( 1 ).value();

        std::cout << "Pushing blocking non-barrier task into queue with priority 1 and value 1." << std::endl;
        if ( !blockingHandle( origin, 2, false, { 1 } ) )
        {
            std::cout << "Execution of the blocking function failed." << std::endl;
        }

        auto nonBlockingHandle = _manager.functions().getFunctionHandle< int, int >( 2 ).value();

        std::cout << "Pushing non-blocking task into queue with priority 3 and value 2." << std::endl;
        if ( !nonBlockingHandle( origin, 3, false, { 2 } ) )
        {
            std::cout << "Execution of the non-blocking function failed." << std::endl;
        }

        std::cout << "Pushing barrier task into queue with priority 5." << std::endl;

        if ( !barrierHandle( origin, 1, false, { } ) )
        {
            std::cout << "Execution of barrier function failed." << std::endl;
        }

        std::cout << "Pushing non-blocking task into queue with priority 10 and value 3" << std::endl;
        if ( !nonBlockingHandle( origin, 10, false, { 3 } ) )
        {
            std::cout << "Execution of non-blockingfunction failed" << std::endl;
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        std::cout << "Function failure in initial received." << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Initial";
    }

    virtual int functionId() const override
    {
        return 0;
    }

    virtual FunctionCompletionType completionType() const override
    {
        return FunctionCompletionType::NonBlocking;
    }

    virtual FunctionDistributionType distributionType() const override
    {
        return FunctionDistributionType::Unicast;
    }

    virtual FunctionType functionType() const override
    {
        return FunctionType::Initial;
    }
};