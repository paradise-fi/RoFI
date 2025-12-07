#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "fizzbuzz.hpp"
#include "naiveBarrier.hpp"

/// @brief The initial function is best used to handle distributed initialization. In this example, it does not do much
/// in terms of initialization, but you may use it to, for example, gather information about the network.
class InitialFunction : public DistributedFunction< int >
{
    int _moduleId;
    DistributedTaskManager& _manager;

public:
    InitialFunction( int moduleId, DistributedTaskManager& manager )
    : _moduleId( moduleId ),
      _manager( manager ) {}

    virtual FunctionResult< int > execute() override
    {
        return FunctionResult< int >( _moduleId, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            std::cout << "No result value" << std::endl;
            return false;
        }

        int moduleId = result.value();

        std::cout << "Initial ModuleId: " << moduleId << std::endl;

        // IMPORTANT!!! - Register participants into the naive barrier.
        auto barrierHandle = _manager.functions().getFunctionHandle< Ip6Addr >( 100 ).value();
        auto& barrierImplementation = static_cast< NaiveBarrier& >( barrierHandle.implementation() );
        barrierImplementation.registerParticipant( origin );

        auto fizzbuzzHandle = _manager.functions().getFunctionHandle< FizzBuzzMetaData, int >( 1 ).value();

        if ( !fizzbuzzHandle( origin, 1, false, std::tuple< int >( moduleId ) ) )
        {
            std::cout << "Execution of function " << functionName() << "failed." << std::endl;
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Initial";
    }

    virtual int functionId() const override
    {
        return 0;
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
        return FunctionType::Initial;
    }
};