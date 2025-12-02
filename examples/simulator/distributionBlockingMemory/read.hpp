#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class Read : public DistributedFunction< int, int >

{
    int _identity;
    DistributedTaskManager& _manager;
    std::map< int, int > _addressStampMap;

public:
    Read( int identity, DistributedTaskManager& manager )
    : _identity( identity ), _manager( manager ) {}

    /// @brief The execute() function is performed by the follower node.
    /// @param value the argument, corresponding to < Arguments > of the template.
    /// @return A function result structure, which contains both the result value and whether the function is considered a success.
    virtual FunctionResult< int > execute( int target ) override 
    {
        std::cout << "Going to read data from address " << target << std::endl;
        auto readResult = _manager.memory().readData( target );
        if ( readResult.success )
        {
            std::cout << "I have received the data succesfully, the data is: " << readResult.data< int >() << std::endl;
            std::cout << "Now requesting the data to be deleted." << std::endl;
            _manager.memory().removeData( target );
            return FunctionResult< int >( target, FunctionResultType::SUCCESS );
        }
        else
        {
            std::cout << "Read failed." << std::endl;
            return FunctionResult< int >( target, FunctionResultType::FAILURE );
        }
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a success.
    virtual bool onFunctionSuccess( std::optional< int > data, const Ip6Addr& origin ) override
    {
        if ( !data.has_value() )    
        {
            return false;
        }

        auto checkHandle = _manager.getFunctionHandle< int, int >( "Check" ).value();
        if ( !checkHandle( origin, 1, false, { data.value() } ) )
        {
            std::cout << "Execution of next function failed." << std::endl;
        }

        return false;
    }

    /// This function is called by the leader node if the FunctionResult fom execute indicates a failure.
    virtual bool onFunctionFailure( std::optional< int >, const Ip6Addr& sender ) override
    {
        std::cout << "Function Failure for " << functionName() << " obtained from " << sender << std::endl;
        return false;
    }

    virtual std::string functionName() const override
    {
        return "Read";
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