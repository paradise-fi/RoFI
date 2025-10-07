// A simple barrier that requires all of the participants to respond.
#include "distributedTaskManager.hpp"
#include "distributedFunction.hpp"

class NaiveBarrier : public DistributedFunction< Ip6Addr >
{
    // Needs to be atomic
    std::set< Ip6Addr > _participants;
    Ip6Addr& _address;
    DistributedTaskManager& _manager;

public:
    NaiveBarrier( Ip6Addr& address, DistributedTaskManager& manager )
    : _address( address ), _manager( manager ) {}

    void registerParticipant( Ip6Addr participant )
    {
        std::ostringstream stream;
        stream << functionName() << " :Registering participant " << participant << std::endl;
        _manager.loggingService().logInfo( stream.str() );

        _participants.insert( participant );
    }

    virtual FunctionResult< Ip6Addr > execute() override {
        _manager.loggingService().logInfo("Executing NaiveBarrier.");
        return FunctionResult< Ip6Addr >( _address, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< Ip6Addr >, const Ip6Addr& origin ) override
    {
        _participants.erase( origin );

        _manager.loggingService().logInfo("NaiveBarrier Success.");
        
        if (_participants.empty())
        {
            _manager.loggingService().logInfo("NaiveBarrier Unblocking.");
            _manager.broadcastUnblockSignal();
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< Ip6Addr >, const Ip6Addr& ) override
    {
        return false;
    }

    virtual std::string functionName() const override
    {
        return "NaiveBarrier";
    }

    virtual int functionId() const override
    {
        return 100;
    }

    // This means that the barrier will not be dequeued upon completion.
    virtual FunctionCompletionType completionType() const override
    {
        return FunctionCompletionType::Blocking;
    }

    // Setting this to broadcast will ensure that the barrier is sent out to all modules when it is invoked.
    virtual FunctionDistributionType distributionType() const override
    {
        return FunctionDistributionType::Broadcast;
    }

    virtual FunctionType functionType() const override
    {
        return FunctionType::Barrier;
    }
};