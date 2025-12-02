#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class SendSave : public DistributedFunction< int, int >
{
    int _identity;
    DistributedTaskManager& _manager;

public:
    SendSave( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager ) {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int target ) override 
    {
        int payload = target;
        std::vector< uint8_t > message( sizeof( int ) );
        std::memcpy( message.data(), &payload, sizeof( int ) );
        Ip6Addr addr = _manager.getLeader().value();
        std::cout << "Going to send out custom blocking message to " << addr << std::endl;
        MessagingResult mr = _manager.sendCustomMessageBlocking( message.data(), message.size(), addr );
        if ( mr.success )
        {
            if ( mr.rawData.size() > 0 )
            {
                std::cout << "Received blocking message response from " << addr << " with payload " << as< int >( mr.rawData.data() ) << std::endl;
            }

            return FunctionResult< int >( target, FunctionResultType::SUCCESS );
        }

        std::cout << "Blocking message failed. Reason: " << mr.statusMessage << std::endl;

        return FunctionResult< int >( target, FunctionResultType::FAILURE );
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > data, const Ip6Addr& origin ) override
    {
        if ( !data.has_value() )    
        {
            return false;
        }

        std::cout << "Received success for " << functionName() << std::endl;

        auto readHandle = _manager.getFunctionHandle< int, int >( "Read" ).value();
        if ( !readHandle( origin, 1, false, std::tuple< int >( data.value() ) ) )
        {
            std::cout << "Execution of next function failed." << std::endl;
        }

        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& origin ) override
    {
        std::cout << "Received failure for function " << functionName() << " from " << origin << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "SendSave";
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

    virtual FunctionType functionType() const override
    {
        return FunctionType::Regular;
    }
};