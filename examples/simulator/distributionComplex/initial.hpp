#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "botState.hpp"


using namespace rofi::hal;

class Initial : public DistributedFunction< ModuleState >
{
    DistributedTaskManager& _manager;
    BotState& _state;

public:
    Initial( DistributedTaskManager& manager, BotState& state )
    : _manager( manager ),
      _state( state ) {}

    virtual FunctionResult< ModuleState > execute() override
    {
        std::cout << "Returning module state for " << _state.moduleAddress << std::endl;
        return FunctionResult< ModuleState >( _state.currentModuleState(), FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< ModuleState > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            std::cout << "No result value" << std::endl;
            return false;
        }

        _state.modules[ origin ] = result.value();
        std::cout << "Received module information from " << origin << " this module is has " << result.value().connectors.size() << " connectors and " << result.value().joints.size() << " joints." << std::endl;

        return false;
    }

    virtual bool onFunctionFailure( std::optional< ModuleState >, const Ip6Addr& ) override
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
};