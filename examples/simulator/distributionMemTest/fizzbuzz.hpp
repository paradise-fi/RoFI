#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class FizzBuzz : public DistributedFunction< int, int >
{
    const int _fizzBuzzTreshold = 40;
    int _identity;
    int _address = 1;
    DistributedTaskManager& _manager;
    std::map< int, int > _addressStampMap;

public:
    FizzBuzz( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager )
    {
        _address += 10 * ( identity - 1 );
    }

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int ) override 
    {
        int address = _address + 10; 
        int result;

        if ( _identity == 2 )
        {
            sleep( 4 );
        }

        if ( ( _identity == 2 && address > 21 ) || ( _identity == 3 && address > 31 ) )
        {
            MemoryReadResult memoryResult = _manager.memoryService().readData( address );
            if ( !memoryResult.success )
            {
                std::cout << "FizzBuzz value not in memory, generating..." << std::endl;
                result = address;
            }
            else
            {
                result = memoryResult.data< int >();
            }
        }
        else
        {
            result = address;
        }
        std::cout << "FizzBuzz Value " << result << " will be stored to memory." << std::endl;
        _manager.memoryService().saveData< int >( result, address );
        _address = address;
        return FunctionResult< int >( result, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > addr, const Ip6Addr& origin ) override
    {
        if ( !addr.has_value() )
        {
            return false;
        }

        if ( _fizzBuzzTreshold <= addr.value() )
        {
            std::cout << "Example complete. Terminating pipeline." << std::endl;
            return false;
        }

        // std::cout << "Function Success, going to read memory" << std::endl;
        // int result;
        // MemoryReadResult memoryResult = _manager.memoryService().readData( addr.value() );

        // std::cout << "After memory read." << std::endl;
        // while ( !memoryResult.success )
        // {
        //     std::cout << "Going to re-queue this task." << std::endl;
        //     return true;
        // }

        int result = addr.value();
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

        auto fizzbuzzHandle = _manager.getFunctionHandle< int, int >( functionId() ).value();
        if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( result ) ) )
        {
            std::cout << "Execution of function " << functionName() << "failed." << std::endl;
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