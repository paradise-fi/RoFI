#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class Check : public DistributedFunction< int, int >
{
    const int _fizzBuzzTreshold = 40;
    int _identity;
    DistributedTaskManager& _manager;
    std::map< int, int > _addressStampMap;

public:
    Check( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager ) {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int target ) override 
    {
        std::cout << "Going to read data from address " << target << " but there should not be any." << std::endl;
        auto readResult = _manager.memory().readData( target );
        if ( readResult.success )
        {
            std::cout << "I have received the data succesfully, the data is: " << readResult.data< int >() << std::endl;
            std::cout << "This is BAD. I should not have received any data!" << std::endl;
            return FunctionResult< int >( target, FunctionResultType::FAILURE );
        }

        std::cout << "No data found. The delete was succesful." << std::endl;
        return FunctionResult< int >( target, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > data, const Ip6Addr& origin ) override
    {
        std::cout << "Function success obtained from " << origin << std::endl;
        std::cout << "Terminating pipeline." << std::endl;
        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& origin ) override
    {
        std::cout << "Function Failure obtained from " << origin << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Check";
    }

    virtual int functionId() const override
    {
        return 3;
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