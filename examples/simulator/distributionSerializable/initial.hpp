#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "naiveBarrier.hpp"
#include "message.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class InitialFunction : public DistributedFunction< int >
{
    int _moduleId;
    DistributedTaskManager& _manager;
    int _followers;
    int _currentFollowers = 0;

public:
    InitialFunction( int moduleId, DistributedTaskManager& manager, int followers )
    : _moduleId( moduleId ),
      _manager( manager ),
      _followers( followers ) {}

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

        ++_currentFollowers;

        if ( _currentFollowers == _followers )
        {
            auto messageHandle = _manager.getFunctionHandle< Message, Message >( 1 ).value();

            if ( !messageHandle( origin, 1, false, std::tuple< Message >( Message( std::string( "Hello, follower." ) ) ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }   
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
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