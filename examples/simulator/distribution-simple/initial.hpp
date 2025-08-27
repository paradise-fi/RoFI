#include "distributionManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "naiveBarrier.hpp"

class Initial : public DistributedFunction< int >
{
    int _moduleId;
    DistributionManager& _manager;

public:
    Initial( int moduleId, DistributionManager& manager )
    : _moduleId( moduleId ),
      _manager( manager ) {}

    virtual FunctionResult< int > execute() override
    {
        return FunctionResult< int >( _moduleId, true );
    }

    virtual void onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            std::cout << "No result value" << std::endl;
            return;
        }

        int moduleId = result.value();

        std::cout << "Initial ModuleId: " << moduleId << std::endl;

        auto fizzbuzzHandle = _manager.getFunctionHandle< int, int >( 1 ).value();

        if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
        {
            std::cout << "Execution of function " << functionName() << "failed." << std::endl;
        }
    }

    virtual void onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return;
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
};