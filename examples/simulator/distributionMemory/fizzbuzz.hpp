#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class FizzBuzz : public DistributedFunction< int, int >
{
    const int _fizzBuzzTreshold = 50;
    int _identity;
    DistributedTaskManager& _manager;
    std::map< int, int > _addressStampMap;

public:
    FizzBuzz( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int value ) override 
    {
        int result;
        std::cout << "Reading memory from address " << _identity << "..." << std::endl;
        MemoryReadResult memoryResult = _manager.memory().readData( _identity );
        if ( !memoryResult.success )
        {
            std::cout << "Value not in memory, generating..." << std::endl;
            result = _identity + value ;
            std::cout << "Value " << _identity << " + " << value;
        }
        else
        {
            // The MemoryReadResult structure provides a useful method for data retrieval using templates.
            result = _identity + memoryResult.data< int >();
            std::cout << "Value " << _identity << " + " << memoryResult.data< int >();
        }
        std::cout << " = " << result << " will be stored to memory." << std::endl;
        _manager.memory().saveData< int >( std::forward< int >( result ), _identity );
        return FunctionResult< int >( _identity, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > addr, const Ip6Addr& origin ) override
    {
        if ( !addr.has_value() )
        {
            return false;
        }

        int result;
        MemoryReadResult memoryResult = _manager.memory().readData( addr.value() );;
        if ( !memoryResult.success )
        {
            std::cout << "Memory entry not found. Going to re-queue this task." << std::endl;
            return true;
        }

        result = memoryResult.data< int >();
        std::cout << origin << ": " << result << " ";
        if ( result % 3 == 0 )
        {
            std::cout << "fizz";
        }
        
        if ( result % 5 == 0 )
        {
            std::cout << "buzz";
        }
        std::cout << std::endl;

        if ( result < _fizzBuzzTreshold )
        {
            auto fizzbuzzHandle = _manager.functions().getFunctionHandle< int, int >( functionId() ).value();
            if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( result ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }
        else
        {
            auto cleanupHandle = _manager.functions().getFunctionHandle< int >( "Cleanup" ).value();
            if ( !cleanupHandle( origin, 1, false, {} ) )
            {
                std::cout << "Execution of Cleanup Function failed." << std::endl;
            }
        }

        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return false;
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

    virtual FunctionType functionType() const override
    {
        return FunctionType::Regular;
    }
};