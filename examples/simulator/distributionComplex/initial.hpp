#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "botState.hpp"


using namespace rofi::hal;

class Initial : public DistributedFunction< ModuleState >
{
    DistributedTaskManager& _manager;
    BotState& _state;
    std::set< Ip6Addr >& _requesters;

public:
    Initial( DistributedTaskManager& manager, BotState& state, std::set< Ip6Addr >& requesters )
    : _manager( manager ),
      _state( state ),
      _requesters( requesters ) {}

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
        _requesters.erase( origin );
        if ( _requesters.empty() )
        {
            std::cout << "Requesters empty. All module information obtained." << std::endl;

            auto result = _state.tryFindLoop();

            if ( result.has_value() )
            {
                std::cout << "Loop found at: " << result->first << ", connector: " << result->second << std::endl;
                auto handle = _manager.functionRegistry().getFunctionHandle< bool, int >( 1 );
                if ( handle.has_value() )
                {
                    handle.value()( result->first, 0, false, { result->second } );
                }
            }
            else
            {
                std::cout << "No loop found!" << std::endl;
            }
        }
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