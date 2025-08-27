#include "distributionManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "naiveBarrier.hpp"

class Initial : public DistributedFunction< int >
{
    int _moduleId;
    DistributionManager& _manager;

public:
    Initial( int moduleId, DistributionManager& manager )
    : _moduleId( moduleId ),
      _manager( manager ) {}

    virtual FunctionResult< int > execute() override
    {
        return FunctionResult< int >( _moduleId, true );
    }

    virtual void onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        std::cout << "Initial.onFunctionSuccess()" << std::endl;
        if ( !result.has_value() )
        {
            std::cout << "No result value" << std::endl;
            return;
        }

        int moduleId = result.value();

        std::cout << "Initial ModuleId: " << moduleId << std::endl;

        auto barrierHandle = _manager.functionRegistry().getFunctionHandle< Ip6Addr >( 100 ).value();
        auto& barrierImplementation = static_cast< NaiveBarrier& >( barrierHandle.implementation() );
        barrierImplementation.registerParticipant( origin );

        std::cout << "Retrieving add" << std::endl;
        auto addHandle = _manager.functionRegistry().getFunctionHandle< int, int >( 1 ).value();

        if ( moduleId % 2 == 0 )
        {
            std::cout << "Calling add" << std::endl;
            if ( !addHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }
        else
        {
            std::cout << "Retrieving multiply" << std::endl;
            auto multiplyHandle = _manager.functionRegistry().getFunctionHandle< int, int >( 2 ).value(); 

            std::cout << "Calling multiply" << std::endl;
            if ( !multiplyHandle( origin, 1, false, std::tuple< int >( 2 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
            std::cout << "Calling add" << std::endl;
            if ( !addHandle( origin, 1, false, std::tuple< int >( 1 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }

        // std::cout << "Sending out Naive Barrier." << std::endl;
    }

    virtual void onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return;
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
};