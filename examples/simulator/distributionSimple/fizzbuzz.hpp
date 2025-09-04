#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class FizzBuzz : public DistributedFunction< int, int >
{
    const int _fizzBuzzTreshold = 600;
    int _identity;
    DistributionManager& _manager;

public:
    FizzBuzz( int identity, DistributionManager& manager )
    : _identity( identity ), _manager( manager )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int value ) override 
    {
        int result = _identity + ( value * _identity );
        std::cout << "FizzBuzz Value:" << result << std::endl;
        return FunctionResult< int >( result, true );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual void onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return;
        }

        int value = result.value();

        
        std::cout << origin << ": " << value << " ";
        if ( value % 3 == 0 )
        {
            std::cout << "fizz";
        }
        
        if ( value % 5 == 0 )
        {
            std::cout << "buzz";
        }
        std::cout << std::endl;

        if ( value < _fizzBuzzTreshold )
        {
            auto fizzbuzzHandle = _manager.functionRegistry().getFunctionHandle< int, int >( functionId() ).value();
            if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( value ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual void onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return;
    }

    virtual std::string functionName() const override
    {
        return "FizzBuzz";
    }

    virtual int functionId() const override
    {
        return 1;
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