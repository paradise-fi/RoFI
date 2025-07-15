#pragma once
#include <functional>
#include <map>
#include <memory>
#include "functionModel.hpp"
#include <networking/networkManager.hpp>

template < typename Result, typename... Arguments >
using FunctionType = FunctionModel< Result, Arguments... >::FunctionType;

template < typename Result, typename... Arguments >
using ReactionType = FunctionModel< Result, Arguments... >::ReactionType;

class FunctionManager
{
    std::map< int, std::unique_ptr< FunctionConcept > > _functions;

public:
    bool isFunctionRegistered( int id ) const
    {
        return _functions.find( id ) != _functions.end();
    }

    template < typename Result, typename... Arguments >
    bool addFunction( int id, 
        FunctionType< Result, Arguments... > function, 
        ReactionType< Result, Arguments... > reaction,
        CompletionType completionType )
    {
        if ( _functions.find( id ) != _functions.end() )
        {
            return false;
        }

        _functions.emplace( id, std::make_unique< FunctionModel< Result, Arguments... > >( id, function, reaction, completionType ) );
        return true;
    }

    bool removeFunction( int id )
    {
        return _functions.erase( id ) != 0;
    }

    bool invokeFunction( TaskBase& task )
    {
        const auto& fn = _functions.find( task.functionId() );
        
        if ( fn == _functions.end() )
        {
            task.setStatus( TaskStatus::Failed );
            return false;
        }

        fn->second->perform( task );
        return true;
    }

    bool invokeReaction( const Ip6Addr& addr, const TaskBase& task )
    {
        const auto& fn = _functions.find( task.functionId() );
        if ( fn == _functions.end() )
        {
            std::cerr << "Function not found when trying to react to module task completion" << std::endl;
            return false;
        }

        fn->second->react( addr, task );
        return true;
    }

    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( int functionId )
    {
        auto kv = _functions.find( functionId );
        
        if ( kv == _functions.end() )
        {
            return std::nullopt;
        }

        return *( kv->second.get() );
    }
};