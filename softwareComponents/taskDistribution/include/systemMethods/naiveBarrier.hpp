#pragma once

// A simple barrier that requires all of the registered participants to respond.
#include "../distributedTaskManager.hpp"
#include "../distributedFunction.hpp"

class NaiveBarrier : public DistributedFunction< Ip6Addr >
{
    std::set< Ip6Addr > _participants;
    std::set< Ip6Addr > _reached;
    Ip6Addr& _address;
    DistributedTaskManager& _manager;

public:
    NaiveBarrier( Ip6Addr& address, DistributedTaskManager& manager )
    : _address( address ), _manager( manager ) {}

    // See the Initial class in examples using this barrier to see how to use this.
    void registerParticipant( Ip6Addr participant )
    {
        std::ostringstream stream;
        stream << functionName() << " :Registering participant " << participant << std::endl;
        _manager.loggingService().logInfo( stream.str(), LogVerbosity::High );

        _participants.insert( participant );
    }

    virtual FunctionResult< Ip6Addr > execute() override {
        _manager.loggingService().logInfo("Reached NaiveBarrier.", LogVerbosity::Medium );
        return FunctionResult< Ip6Addr >( _address, FunctionResultType::SUCCESS );
    }

    virtual bool onFunctionSuccess( std::optional< Ip6Addr >, const Ip6Addr& origin ) override
    {
        _reached.insert( origin );

        std::ostringstream stream;
        stream << "NaiveBarrier reached by " << origin;
        _manager.loggingService().logInfo( stream.str(), LogVerbosity::Medium );
        
        if (_reached == _participants)
        {
            _reached.clear();
            _manager.loggingService().logInfo( "NaiveBarrier Unblocking.", LogVerbosity::Medium );
            _manager.broadcastUnblockSignal();
        }

        return false;
    }

    virtual bool onFunctionFailure( std::optional< Ip6Addr >, const Ip6Addr& ) override
    {
        _manager.loggingService().logError( "Naive barrier failure. This function should never be invoked." );
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

    // A barrier function cannot be registered if completionType() is not set to FunctionCompletionType::Blocking!
    // Non-barrier functions' tasks with this completion type will prevent additional tasks from being scheduled until
    // the scheduler is cleared manually. However, unlike a barrier function tasks, regular but blocking tasks can be
    // overtaken by tasks with higher priority.
    virtual FunctionCompletionType completionType() const override
    {
        return FunctionCompletionType::Blocking;
    }

    virtual FunctionDistributionType distributionType() const override
    {
        return FunctionDistributionType::Unicast;
    }

    // This is crucial, otherwise the function would not be registered as a barrier!
    virtual FunctionType functionType() const override
    {
        return FunctionType::Barrier;
    }
};