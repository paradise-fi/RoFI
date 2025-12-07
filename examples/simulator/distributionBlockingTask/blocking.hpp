#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class BlockingFunction : public DistributedFunction< int, int >
{
    int _moduleId;
    DistributedTaskManager& _manager;
    bool& _nonBlockingFirstFlag;

public:
    BlockingFunction( int moduleId, DistributedTaskManager& manager, bool& nonBlockingFirstFlag )
    : _moduleId( moduleId ), _manager( manager ), _nonBlockingFirstFlag( nonBlockingFirstFlag ) {}

    virtual FunctionResult< int > execute( int value ) override
    {
        std::cout << "[" << functionName() << "] Executing blocking function with value " << value << std::endl;
        if ( _nonBlockingFirstFlag )
        {
            _nonBlockingFirstFlag = false;
            std::cout << "[" << functionName() << "] The non blocking task was called with higher priority before me. Success." << std::endl;
            return FunctionResult< int >( _moduleId, FunctionResultType::SUCCESS );
        }

        std::cout << "[" << functionName() << "] The non-blocking task was not yet called with higher priority. Failure." << std::endl;
        return FunctionResult< int >( _moduleId, FunctionResultType::FAILURE );
    }

    virtual bool onFunctionSuccess( std::optional< int >, const Ip6Addr& origin) override
    {
        std::cout << "[" << functionName() << "]  Function Success. Sending back a soft unblock signal." << std::endl;
        _manager.sendUnblockSignal( origin );
        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        std::cout << "[" << functionName() << "]  Function Failure." << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Blocking";
    }

    virtual int functionId() const override
    {
        return 1;
    }

    virtual FunctionCompletionType completionType() const override
    {
        return FunctionCompletionType::Blocking;
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