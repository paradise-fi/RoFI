#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "botState.hpp"


using namespace rofi::hal;

class Disconnect : public DistributedFunction< int, int >
{
    DistributedTaskManager& _manager;
    BotState& _state;

public:
    Disconnect( DistributedTaskManager& manager, BotState& state )
    : _manager( manager ),
      _state( state ) {}

    virtual FunctionResult< int > execute( int connectorId ) override
    {
        std::cout << "Disconnect with connectorId " << connectorId << std::endl;
        auto rofi = RoFI::getLocalRoFI();
        std::cout << "Got local rofi " << std::endl;
        rofi.getConnector( connectorId ).disconnect();
        std::cout << "Disconnected" << std::endl;
        return FunctionResult< int >( connectorId, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< int > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return false;
        }
        std::cout << "Disconnected connector ID " << result.value() << " on " << origin << std::endl;
        auto oppositeModule = _state.modules.at( origin ).connectors[ result.value() ].connectedTo.value();
        auto oppositeConnector = _state.modules.at( origin ).connectors[ result.value() ].otherSideConnectorId.value();
        _state.modules.at( origin ).connectors[ result.value() ].connectedTo = std::nullopt;
        _state.modules.at( origin ).connectors[ result.value() ].otherSideConnectorId = std::nullopt;
        _state.modules.at( oppositeModule ).connectors[ oppositeConnector ].otherSideConnectorId = std::nullopt;
        _state.modules.at( oppositeModule ).connectors[ oppositeConnector ].connectedTo = std::nullopt;
        return false;
    }

    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Disconnect";
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