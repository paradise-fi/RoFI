// A simple barrier that requires all of the participants to respond.
#include "distributionManager.hpp"
#include "distributedFunction.hpp"

class NaiveBarrier : public DistributedFunction< Ip6Addr >
{
    // Needs to be atomic
    std::set< Ip6Addr > _participants;
    Ip6Addr& _address;
    DistributionManager& _manager;

public:
    NaiveBarrier( Ip6Addr& address, DistributionManager& manager )
    : _address( address ), _manager( manager ) {}

    void registerParticipant( Ip6Addr participant )
    {
        std::cout << functionName() << " :Registering participant " << participant << std::endl;
        _participants.insert( participant );
    }

    virtual FunctionResult< Ip6Addr > execute() override {
        std::cout << "Naive Barrier execution." << std::endl;
        return FunctionResult< Ip6Addr >( _address, true );
    }

    virtual void onFunctionSuccess( std::optional< Ip6Addr >, const Ip6Addr& origin ) override
    {
        _participants.erase( origin );

        if (_participants.empty())
        {
            _manager.broadcastUnblockSignal();
        }
    }

    virtual void onFunctionFailure( std::optional< Ip6Addr >, const Ip6Addr& ) override
    {
        return;
    }

    virtual std::string functionName() const override
    {
        return "NaiveBarrier";
    }

    virtual int functionId() const override
    {
        return 100;
    }

    virtual FunctionCompletionType completionType() const override
    {
        return FunctionCompletionType::Blocking;
    }

    virtual FunctionDistributionType distributionType() const override
    {
        return FunctionDistributionType::Broadcast;
    }
};