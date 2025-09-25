#pragma once

#include "distributionManager.hpp"
#include "distributedFunction.hpp"

struct FizzBuzzMetaData {
    int value;
    int identity;
};

class FizzBuzz : public DistributedFunction< FizzBuzzMetaData, int >
{
    const int _fizzBuzzTreshold = 20;
    int _identity;
    const int _memorySlotOneId = 2;
    const int _memorySlotTwoId = 3;
    DistributionManager& _manager;
    std::map< int, int > _addressStampMap;

    int getMemorySlotAddress( int value )
    {
        return value % 2 == 0 ? _memorySlotOneId : _memorySlotTwoId;
    }

public:
    FizzBuzz( int identity, DistributionManager& manager )
    : _identity( identity ), _manager( manager )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< FizzBuzzMetaData > execute( int value ) override 
    {
        int memoryAddress = getMemorySlotAddress( value );
        std::cout << "FizzBuzz Value " << value << " Memory Address: " << memoryAddress << std::endl;
        MemoryReadResult result = _manager.memoryService().readData( memoryAddress );
        int resultValue;
        if ( !result.success )
        {
            std::cout << "FizzBuzz value not in memory, generating..." << std::endl;
            resultValue = _identity + ( value * _identity );
        }
        else
        {
            resultValue = _identity + ( result.data< int >() * _identity );
        }
        std::cout << "FizzBuzz Value " << resultValue << " will be stored to memory." << std::endl;
        _manager.memoryService().saveData< int >( resultValue, memoryAddress );

        auto metaData = FizzBuzzMetaData{ value + 1, _identity };
        std::cout << "Returning metadata with value " << value + 1 << std::endl;
        return FunctionResult< FizzBuzzMetaData >( metaData, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< FizzBuzzMetaData > data, const Ip6Addr& origin ) override
    {
        if ( !data.has_value() )
        {
            return false;
        }

        std::cout << "Data Value: " << data.value().value << std::endl;
        int memoryAddress = getMemorySlotAddress( data.value().value );

        std::cout << "Memory Address: " << memoryAddress << std::endl;
        MemoryReadResult readResult = _manager.memoryService().readData( memoryAddress );
        if ( !readResult.success )
        {
            return true;
        }
        
        int result = readResult.data< int >();
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

        if ( data.value().identity == 2 )
        {
            auto barrierHandle = _manager.functionRegistry().getFunctionHandle< Ip6Addr >( 100 ).value();
            if ( !barrierHandle( origin, 1, false, std::tuple<>()) )
            {
                std::cout << "Execution of function " << functionName() << " failed" << std::endl;
            }
        }

        if ( data.value().value < _fizzBuzzTreshold )
        {
            auto fizzbuzzHandle = _manager.functionRegistry().getFunctionHandle< FizzBuzzMetaData, int >( 1 ).value();
            if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( data.value().value + 1 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }

        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< FizzBuzzMetaData >, const Ip6Addr& ) override
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
};