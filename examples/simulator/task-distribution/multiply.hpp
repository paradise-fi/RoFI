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
        std::cout << _value << " * " << multiplyValue;
        _value *= multiplyValue;
        std::cout << " = " << _value << std::endl;

        int value = 300 + _value;
        if ( !_manager.saveData( reinterpret_cast< uint8_t* >( &value ), sizeof( int ), 3 ) )
        {
            return FunctionResult< int >( _value, false );
        }

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
            auto addHandle = _manager.getFunctionHandle< int, int >( 1 ).value();
            if ( !addHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
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