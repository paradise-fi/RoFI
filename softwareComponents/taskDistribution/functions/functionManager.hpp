#pragma once
#include <functional>
#include <map>
#include <memory>
#include "functionModel.hpp"
#include "distributedFunction.hpp"
#include "functionHandle.hpp"
#include <networking/networkManager.hpp>

template < typename Result, typename... Arguments >
using Executor = std::function< bool( const Ip6Addr&, int, bool, FunctionModel< Result, Arguments...>& fn, std::tuple< Arguments...> && arguments ) >;

class FunctionManager
{
    std::map< int, std::unique_ptr< FunctionConcept > > _functions;
    std::map< std::string, int > _nameToIdMap;

public:
    bool isFunctionRegistered( int id ) const
    {
        return _functions.find( id ) != _functions.end();
    }

    template < typename Result, typename... Arguments >
    bool addFunction( 
        std::unique_ptr< DistributedFunction< Result, Arguments... > > userFunction )
    {
        int id = userFunction->functionId();
        std::string name = userFunction->functionName();

        if ( _functions.find( id ) != _functions.end() )
        {
            return false;
        }

        if ( _nameToIdMap.find( name ) != _nameToIdMap.end() )
        {
            return false;
        }

        _functions.emplace( id, std::make_unique< FunctionModel< Result, Arguments... > >( std::move( userFunction ) ) );

        _nameToIdMap.emplace( name, id );
        return true;
    }

    bool removeFunction( int id )
    {
        auto fnCandidate = _functions.find( id );

        if ( fnCandidate == _functions.end() )
        {
            return false;
        }

        _nameToIdMap.erase( fnCandidate->second->functionName() );

        return _functions.erase( id ) != 0;
    }

    bool removeFunction( std::string name )
    {
        auto idCandidate = _nameToIdMap.find( name );
        
        if ( idCandidate == _nameToIdMap.end() )
        {
            return false;
        }

        return removeFunction( idCandidate->second );
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

    FunctionResultType invokeReaction( const Ip6Addr& addr, const TaskBase& task )
    {
        const auto& fn = _functions.find( task.functionId() );
        if ( fn == _functions.end() )
        {
            std::cerr << "Function not found when trying to react to module task completion" << std::endl;
            return FunctionResultType::FAILURE;
        }

        if ( task.status() != TaskStatus::Complete )
        {
            return fn->second->onFailure( addr, task ) ? FunctionResultType::TRY_AGAIN : FunctionResultType::SUCCESS;
        }
        
        return fn->second->onSuccess( addr, task ) ? FunctionResultType::TRY_AGAIN : FunctionResultType::SUCCESS;
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

    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( std::string functionName )
    {
        auto idCandidate = _nameToIdMap.find( functionName );
        
        if ( idCandidate == _nameToIdMap.end() )
        {
            return std::nullopt;
        }

        return getFunction( idCandidate->second );
    }
};