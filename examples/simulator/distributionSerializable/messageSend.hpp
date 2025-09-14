#include "distributionManager.hpp"
#include "distributedFunction.hpp"
#include "message.hpp"

/// @brief This DistributedFunction uses a Serializable structure, Message, which implements methods that serialize a string into network messages.
class MessageSend : public DistributedFunction< Message, Message >
{
    DistributionManager& _manager;
    int _moduleId;

public:
    MessageSend( DistributionManager& manager, int moduleId )
    : _manager( manager ), _moduleId( moduleId )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< Message > execute( Message message ) override 
    {
        if ( message.message == std::string("end"))
        {
            return FunctionResult< Message >( Message( std::string( "end" ) ), true );
        }

        std::cout << "Received value from leader: " << message.message << std::endl;
        std::string hello("Hi from ");
        return FunctionResult< Message >( Message( hello + std::to_string( _moduleId ) ), true );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual void onFunctionSuccess( std::optional< Message > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return;
        }

        Message value = result.value();

        std::cout << "Received message \"" << value.message << "\" from " << origin << std::endl;

        if ( value.message != std::string( "end" ) )
        {
            auto messageSendHandle = _manager.functionRegistry().getFunctionHandle< Message, Message >( functionId() ).value();
            if ( !messageSendHandle( origin, 1, false, { std::string( "end" ) } ) )
            {
                std::cout << "Execution of function " << functionName() << "failed." << std::endl;
            }
            return;
        }
        std::cout << "Function end." << std::endl;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual void onFunctionFailure( std::optional< Message >, const Ip6Addr& ) override
    {
        return;
    }

    virtual std::string functionName() const override
    {
        return "MessageSend";
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