#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class Multiply : public DistributedFunction< int, int >
{
    int& _value;
    DistributedTaskManager& _manager;

public:
    Multiply( int& baseValue, DistributedTaskManager& manager )
    : _value( baseValue ), _manager( manager ) {}

    virtual FunctionResult< int > execute( int multiplyValue ) override
    {
        std::cout << _value << " * " << multiplyValue;
        _value *= multiplyValue;
        std::cout << " = " << _value << std::endl;

        int value = 300 + _value;

        if ( !_manager.memoryService().saveData< int >( value, 3 ) )
        {
            return FunctionResult< int >( _value, FunctionResultType::FAILURE );
        }

        return FunctionResult< int >( _value, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return false;
        }

        int value = result.value();

        std::cout << "Received Multiply Result " << value << " from " << origin << std::endl;

        if ( value < 5 )
        {
            auto addHandle = _manager.functionRegistry().getFunctionHandle< int, int >( 1 ).value();
            if ( !addHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
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
        return "Multiply";
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
};