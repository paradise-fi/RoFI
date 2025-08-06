#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class Add : public DistributedFunction< int, int >
{
    int& _value;
    DistributionManager& _manager;

public:
    Add( int& baseValue, DistributionManager& manager )
    : _value( baseValue ), _manager( manager )
    {}

    virtual FunctionResult< int > execute( int value ) override 
    {
        std::cout << _value << " + " << value;
        _value += value;
        std::cout << " = " << _value << std::endl;
        return FunctionResult< int >( _value, true );
    }

    virtual void onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return;
        }

        int value = result.value();

        std::cout << "Received Add Result " << value << " from " << origin << std::endl;

         if ( value < 5 )
        {
            auto addHandle = _manager.getFunctionHandle< int, int >( functionId() ).value();
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
        return "Add";
    }

    virtual int functionId() const override
    {
        return 1;
    }

    virtual CompletionType completionType() const override
    {
        return CompletionType::NonBlocking;
    }
};