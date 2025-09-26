#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"
#include "message.hpp"

/// @brief This DistributedFunction uses a Serializable structure, Message, which implements methods that serialize a string into network messages.
class MessageSend : public DistributedFunction< Message, Message >
{
    DistributedTaskManager& _manager;
    int _moduleId;

public:
    MessageSend( DistributedTaskManager& manager, int moduleId )
    : _manager( manager ), _moduleId( moduleId )
    {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< Message > execute( Message message ) override 
    {
        if ( message.message == std::string("end"))
        {
            return FunctionResult< Message >( Message( std::string( "end" ) ), FunctionResultType::SUCCESS );
        }

        std::cout << "Received value from leader: " << message.message << std::endl;
        std::string hello("Hi from ");
        return FunctionResult< Message >( Message( hello + std::to_string( _moduleId ) ), FunctionResultType::SUCCESS );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< Message > result, const Ip6Addr& origin ) override
    {
        if ( !result.has_value() )
        {
            return false;
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
            return false;
        }
        std::cout << "Function end." << std::endl;
        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< Message >, const Ip6Addr& ) override
    {
        return false;
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