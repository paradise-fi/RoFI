#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class Delay : public DistributedFunction< int, int >
{
    int& _value;
    DistributionManager& _manager;

public:
    Delay( int& baseValue, DistributionManager& manager )
    : _value( baseValue ), _manager( manager )
    {}

    virtual FunctionResult< int > execute( int ) override 
    {
        for (int i = 0; i < 10; ++i)
        {
            sleep( 4 );
            std::cout << "SLEEP: " << i << std::endl;
        }
        return FunctionResult< int >( _value, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        std::cout << "DELAY SUCCESS" << std::endl;
        if ( !result.has_value() )
        {
            return false;
        }

        int value = result.value();

        std::cout << "Received Delay Result " << value << " from " << origin << std::endl;

        auto addHandle = _manager.functionRegistry().getFunctionHandle< int, int >( 1 ).value();
        if ( !addHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
        {
            std::cout << "Execution of function " << functionName() << "failed." << std::endl;
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Delay";
    }

    virtual int functionId() const override
    {
        return 4;
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