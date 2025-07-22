#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class Multiply : public DistributedFunction< int, int >
{
    int& _value;
    DistributionManager& _manager;

public:
    Multiply( int& baseValue, DistributionManager& manager )
    : _value( baseValue ), _manager( manager ) {}

    virtual FunctionResult< int > execute( int multiplyValue ) override
    {
        _value *= multiplyValue;

        return FunctionResult< int >( _value, true );
    }

    virtual void onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return;
        }

        int value = result.value();

        std::cout << "Received Multiply Result " << value << " from " << origin << std::endl;

        if ( value < 5 )
        {
            if ( !_manager.executeFunction< int, int >( origin, 1, false, 1, std::tuple< int >( 1 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }
    }

    virtual void onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return;
    }

    virtual std::string functionName() const override
    {
        return "Multiply";
    }

    virtual int functionId() const override
    {
        return 2;
    }
};