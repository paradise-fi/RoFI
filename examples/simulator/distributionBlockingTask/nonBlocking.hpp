#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class NonBlockingFunction : public DistributedFunction< int, int >
{
    int _moduleId;
    DistributedTaskManager& _manager;
    bool& _nonBlockingFirstFlag;

public:
    NonBlockingFunction( int moduleId, DistributedTaskManager& manager, bool& nonBlockingFirstFlag )
    : _moduleId( moduleId ), _manager( manager ), _nonBlockingFirstFlag( nonBlockingFirstFlag ) {}

    virtual FunctionResult< int > execute( int value ) override
    {
        std::cout << "[" << functionName() << "] Executing non-blocking function with value " << value << std::endl;
        _nonBlockingFirstFlag = true;
        return FunctionResult< int >( _moduleId, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int >, const Ip6Addr& ) override
    {
        std::cout << "[" << functionName() << "]  Function Success." << std::endl;
        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        std::cout << "[" << functionName() << "]  Function Failure." << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "NonBlocking";
    }

    virtual int functionId() const override
    {
        return 2;
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