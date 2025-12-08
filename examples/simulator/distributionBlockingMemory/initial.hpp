#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "systemMethods/naiveBarrier.hpp"

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

        int moduleId = result.value();

        std::cout << "Initial ModuleId: " << moduleId << std::endl;

        if ( moduleId > 2 )
        {
            std::cout << "This module will not execute any other distributed functions." << std::endl;
            return false;
        }

        auto sendSaveHandle= _manager.functions().getFunctionHandle< int, int >( 1 ).value();

        if ( !sendSaveHandle( origin, 1, false, { 3 } ) )
        {
            std::cout << "Execution of the next function failed." << std::endl;
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