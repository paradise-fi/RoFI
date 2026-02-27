#pragma once

#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include "botState.hpp"
#include "moveResult.hpp"

using namespace rofi::hal;

class Move : public DistributedFunction< MoveResult, int, float, float >
{
    DistributedTaskManager& _manager;
    BotState& _state;

public:
    Move( DistributedTaskManager& manager, BotState& state )
    : _manager( manager ),
      _state( state ) {}

    virtual FunctionResult< MoveResult > execute( int jointId, float velocity, float position ) override
    {
        std::cout << "Move with jointId " << jointId << std::endl;
        std::cout << "Velocity: " << velocity << ", Position: " << position << "rad" << std::endl;
        auto rofi = RoFI::getLocalRoFI();
        rofi.getJoint( jointId ).setPosition( position, velocity, [](Joint){ std::cout << "Done" << std::endl;});
        return FunctionResult< MoveResult >( MoveResult{ jointId, position }, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< MoveResult > result, const Ip6Addr& ) override
    {
        if ( !result.has_value() )
        {
            return false;
        }
        // TODO: Update information
        return false;
    }

    virtual bool onFunctionFailure( std::optional< MoveResult >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Move";
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