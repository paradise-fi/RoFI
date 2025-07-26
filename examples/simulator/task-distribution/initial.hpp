#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class Initial : public DistributedFunction< int >
{
    int _moduleId;
    DistributionManager& _manager;

public:
    Initial( int moduleId, DistributionManager& manager )
    : _moduleId( moduleId ), _manager( manager ) {}

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

        if ( moduleId % 2 == 0 )
        {
            if ( !_manager.executeFunction< int, int >( origin, 1, false, 1, std::tuple< int >( 1 ) ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
        }
        else
        {
            if ( !_manager.executeFunction<int, int >( origin, 1, false, 2, std::tuple< int >( 2 ) ) )
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
        return "Initial";
    }

    virtual int functionId() const override
    {
        return 0;
    }
};