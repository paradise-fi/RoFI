#pragma once

#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

struct FizzBuzzMetaData {
    int value;
    int identity;
};

struct MemoryTrackingData
{
    int stamp;
    int staleCount = 0;

    MemoryTrackingData() : stamp( -1 ) {}
    MemoryTrackingData( int stamp ) : stamp( stamp ) {}
};

class FizzBuzz : public DistributedFunction< FizzBuzzMetaData, int >
{
    const int fizzbuzzLimit = 20;
    int fizzBuzzOps = 0;
    int _identity;
    std::map< int, MemoryTrackingData > _prevStamps;
    const int _memorySlotOneId = 2;
    const int _memorySlotTwoId = 3;
    DistributedTaskManager& _manager;
    std::map< int, int > _addressStampMap;

    int getNextMemorySlotAddress( int value )
    {
        return value == _memorySlotOneId ? _memorySlotTwoId : _memorySlotOneId;
    }

public:
    FizzBuzz( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager )
    {
        _prevStamps.emplace(_memorySlotOneId, MemoryTrackingData( -1 ) );
        _prevStamps.emplace(_memorySlotTwoId, MemoryTrackingData( -1 ) );
    }

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< FizzBuzzMetaData > execute( int memoryAddress ) override 
    {
        MemoryReadResult result = _manager.memory().readData( memoryAddress );
        int resultValue;
        if ( !result.success )
        {
            resultValue = _identity + ( memoryAddress * _identity );
            std::cout << "Fizzbuzz value not in memory at address " << memoryAddress << ", generated and stored " << _identity << " + (" << memoryAddress << " * " << _identity << ") = "  << resultValue << " there." << std::endl;
        }
        else
        {
            int previous = result.data< int >();
            resultValue = _identity + ( previous * _identity );
            std::cout << "Fizbuzz value " << previous << " found in memory slot " << memoryAddress << ", updating and storing value " << _identity << " + (" << previous << " * " << _identity << ") = "  << resultValue << " there." << std::endl;
        }

        _manager.memory().saveData< int >( std::forward< int >( resultValue ), memoryAddress );

        auto metaData = FizzBuzzMetaData{ memoryAddress, _identity };
        return FunctionResult< FizzBuzzMetaData >( metaData, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< FizzBuzzMetaData > data, const Ip6Addr& origin ) override
    {
        if ( fizzbuzzLimit <= fizzBuzzOps )
        {
            std::cout << "Example complete for " << origin << ". Terminating pipeline." << std::endl;
            return false;
        }
        
        if ( !data.has_value() )
        {
            return false;
        }

        int memoryAddress = data.value().value;
        std::cout << "FizzBuzz result notification received from " << data.value().identity << ", going to read from address " << memoryAddress << std::endl;
        MemoryReadResult metadata = _manager.memory().readMetadata( memoryAddress, "stamp" );
        if ( !metadata.success )
        {
            std::cout << "Failed to read metadata." << std::endl;
            return true;
        }

        auto& prevStamp = _prevStamps[ memoryAddress ];
        if ( metadata.data< int >() <= prevStamp.stamp && prevStamp.staleCount < 4 )
        {
            prevStamp.staleCount++;
            std::cout << "Stale data detected." << std::endl;
            return true;
        }

        MemoryReadResult readResult = _manager.memory().readData( memoryAddress );
        if ( !readResult.success )
        {
            return true;
        }
        
        fizzBuzzOps++;
        prevStamp.stamp = metadata.data< int >();
        prevStamp.staleCount = 0;
        int result = readResult.data< int >();
        std::cout << "[Result on " << origin << "]: " << result << " -> ";
        if ( result % 3 == 0 )
        {
            std::cout << "fizz";
        }
        
        if ( result % 5 == 0 )
        {
            std::cout << "buzz";
        }
        std::cout << std::endl;

        auto barrierHandle = _manager.functions().getFunctionHandle< Ip6Addr >( 100 ).value();
        if ( !barrierHandle( origin, 1, false, std::tuple<>()) )
        {
            std::cout << "Execution of barrier function failed." << std::endl;
        }

        auto fizzbuzzHandle = _manager.functions().getFunctionHandle< FizzBuzzMetaData, int >( 1 ).value();
        if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( getNextMemorySlotAddress( data.value().value ) ) ) )
        {
            std::cout << "Execution of function " << functionName() << "failed." << std::endl;
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

    virtual FunctionType functionType() const override
    {
        return FunctionType::Regular;
    }
};