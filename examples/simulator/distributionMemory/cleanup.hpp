#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class Cleanup : public DistributedFunction< int >
{
    const int _fizzBuzzTreshold = 100;
    int _identity;
    DistributedTaskManager& _manager;

public:
    Cleanup( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute() override 
    {
        std::cout << "Removing address " << _identity << " from memory." << std::endl;
        _manager.memory().removeData( _identity );
        std::cout << "==============[PIPELINE TERMINATED]==============" << std::endl;
        return FunctionResult< int >( _identity, FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > addr, const Ip6Addr& origin ) override
    {
        if ( !addr.has_value() )
        {
            return false;
        }

        MemoryReadResult memoryResult = _manager.memory().readData( addr.value() );;
        if ( !memoryResult.success )
        {
            std::cout << "Entry under address " << addr.value() << " succesfully deleted." << std::endl;
        }

        std::cout << "Terminating pipeline for " << origin << std::endl;
        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Cleanup";
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

    virtual FunctionType functionType() const override
    {
        return FunctionType::Regular;
    }
};