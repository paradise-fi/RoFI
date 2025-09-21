#pragma once
#include <functional>
#include <memory>
#include <any>

#include "distributedFunction.hpp"
#include "task.hpp"

class FunctionConcept {
    public:
        virtual ~FunctionConcept() = default;
        virtual std::unique_ptr< TaskBase > createTask() const = 0;
        virtual void perform( TaskBase& task ) = 0;
        virtual bool onSuccess( const Ip6Addr& addr, const TaskBase& task ) = 0;
        virtual bool onFailure( const Ip6Addr& addr, const TaskBase& task ) = 0;
        virtual FunctionCompletionType completionType() const = 0;
        virtual FunctionDistributionType distributionType() const = 0;
        virtual std::string functionName() const = 0;
        virtual int functionId() const = 0;
    };

    template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    class FunctionModel : public FunctionConcept
    {
        std::unique_ptr< DistributedFunction< Result, Arguments... > > _function;

    public:
        FunctionModel( std::unique_ptr< DistributedFunction< Result, Arguments... > > fn )
        : _function( std::move( fn ) )
        {}

        DistributedFunction< Result, Arguments... >& getImplementation()
        {
            return *_function.get();
        }
        
        virtual void perform( TaskBase& task ) override
        {
            Task< Result, Arguments... >& specializedTask = dynamic_cast< Task< Result, Arguments... >& >( task );
            auto result = std::apply(
                [ & ]( auto&&... args ) { 
                    return _function->execute( std::forward< decltype( args ) >( args )... ); 
                },
                specializedTask.arguments()
            );

            specializedTask.setResult( result.result );

            if ( result.isSuccessful() )
            {
                specializedTask.setStatus( TaskStatus::Complete );
                return;
            }

            if ( result.shouldFollowerReschedule() )
            {
                specializedTask.setStatus( TaskStatus::RepeatLocally );
                return;
            }

            if ( result.shouldLeaderReschedule() )
            {
                specializedTask.setStatus( TaskStatus::RepeatDistributed );
                return;
            }

            specializedTask.setStatus( TaskStatus::Failed );
        }

        virtual bool onSuccess( const Ip6Addr& addr, const TaskBase& task ) override
        {
            const Task< Result, Arguments... >& specializedTask = dynamic_cast< const Task< Result, Arguments... >& >( task );   
            return _function->onFunctionSuccess( specializedTask.result(), addr );
        }

        virtual bool onFailure( const Ip6Addr& addr, const TaskBase& task ) override
        {
            const Task< Result, Arguments... >& specializedTask = dynamic_cast< const Task< Result, Arguments... >& >( task );   
            return _function->onFunctionFailure( specializedTask.result(), addr );
        }

        std::unique_ptr< TaskBase > createTask() const override
        {
            return std::make_unique< Task<Result, Arguments... > >( _function->functionId() );
        }

        virtual FunctionCompletionType completionType() const override
        {
            return _function.get()->completionType();
        }

        virtual FunctionDistributionType distributionType() const override
        {
            return _function.get()->distributionType();
        }

        virtual std::string functionName() const override
        {
            return _function->functionName();
        }

        virtual int functionId() const override
        {
            return _function->functionId();
        }
    };