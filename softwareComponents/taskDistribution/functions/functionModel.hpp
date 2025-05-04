#pragma once
#include <functional>
#include <memory>
#include <any>

#include "task.hpp"

class FunctionConcept {
    public:
        virtual ~FunctionConcept() = default;
        virtual std::unique_ptr< TaskBase > createTask() const = 0;
        virtual void perform( TaskBase& task ) = 0;
        virtual void react( const Ip6Addr& addr, const TaskBase& task ) = 0;
    };

    template < typename Result, typename... Arguments >
    class FunctionModel : public FunctionConcept
    {
    public:
        using FunctionType = std::function< Result ( Arguments... ) >;
        using ReactionType = std::function< void( std::optional< Result >, const rofi::net::Ip6Addr& ) >;

        FunctionModel( int id, FunctionType fn, ReactionType react_fn )
        : _id( id ),
          _function( fn ), 
          _reaction( react_fn ) 
        {}

        
        virtual void perform( TaskBase& task ) override
        {
            Task< Result, Arguments... >& specializedTask = dynamic_cast< Task< Result, Arguments... >& >( task );
            specializedTask.setResult( invoke( specializedTask.arguments() ) );
            specializedTask.setStatus( TaskStatus::Complete );
        }

        virtual void react( const Ip6Addr& addr, const TaskBase& task ) override
        {
            const Task< Result, Arguments... >& specializedTask = dynamic_cast< const Task< Result, Arguments... >& >( task );   
            _reaction( specializedTask.result(), addr );
        }

        std::unique_ptr< TaskBase > createTask() const override
        {
            return std::make_unique< Task<Result, Arguments... > >( _id );
        }

    private:
        template < std::size_t... I >
        Result invoke( const std::tuple< Arguments... >& arguments ) 
        {
            FunctionType fnCopy = _function;
            return std::apply( std::move( fnCopy ), arguments );
        }

        int _id;
        FunctionType _function;
        ReactionType _reaction;
    };