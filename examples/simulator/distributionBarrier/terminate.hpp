#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "fizzbuzz.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class TerminateFunction : public DistributedFunction< bool >
{
    bool& _terminate;
    DistributedTaskManager& _manager;

public:
    TerminateFunction( bool& terminate, DistributedTaskManager& manager )
    : _terminate( terminate ),
      _manager( manager ) {}

    virtual FunctionResult< bool > execute() override
    {
        std::cout << "Terminate" << std::endl;
        _terminate = true;
        return FunctionResult< bool >( true, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< bool >, const Ip6Addr& ) override
    {
        std::cout << "Terminate" << std::endl;
        _terminate = true;
        return false;
    }

    virtual bool onFunctionFailure( std::optional< bool >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Terminate";
    }

    virtual int functionId() const override
    {
        return 101;
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
        return FunctionType::Regular;
    }
};