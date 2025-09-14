#pragma once
#include "functionResult.hpp"
#include "enums/functionCompletionType.hpp"
#include "enums/functionDistributionType.hpp"
#include <optional>

template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
class DistributedFunction
{
public:
    virtual ~DistributedFunction() = default;

    /// @brief Invoked remotely by the distribution system after the function call is sent out to a module.
    /// @param ...args - arguments of the function, typically received from the leader module.
    /// @return - return value of the function, this will be sent back to the leader module. Setting false to 
    virtual FunctionResult< Result > execute( Arguments... args ) = 0;

    /// @brief Invoked on the leader module after receiving a success result from its follower module.
    /// @param result The result obtained from the follower module.
    /// @param origin The address of the module that performed the function call.
    virtual void onFunctionSuccess( std::optional< Result > result, const Ip6Addr& origin ) = 0;

    /// @brief Invoked on the leader module after receiving a failure result from its follower module.
    /// @param result The result value obtained from the module.
    /// @param origin The address of the module that performed the function call.
    virtual void onFunctionFailure( std::optional< Result > result, const Ip6Addr& origin ) = 0;

    /// @brief Returns unique function name for user-friendly function retrieval.
    /// @return Unique function name.
    virtual std::string functionName() const = 0;

    /// @brief Returns unique function id.
    /// @return Unique function id.
    virtual int functionId() const = 0;

    /// @brief Returns the completion type for the function, determining whether it is blocking after completion or not.
    /// @return The completion type.
    virtual FunctionCompletionType completionType() const = 0;

    /// @brief Determines the way the function will be distributed to followers.
    /// @return The distribution type.
    virtual FunctionDistributionType distributionType() const = 0;
};