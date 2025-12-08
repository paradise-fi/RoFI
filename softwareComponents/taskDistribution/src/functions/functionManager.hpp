#pragma once
#include <functional>
#include <map>
#include <memory>
#include "functionModel.hpp"
#include "distributedFunction.hpp"
#include "../../include/functionHandle.hpp"
#include <networking/networkManager.hpp>
#include "../logger/loggingService.hpp"
#include "lwip++.hpp"

class FunctionManager
{
    std::map< int, std::unique_ptr< FunctionConcept > > _functions;
    std::map< std::string, int > _nameToIdMap;
    LoggingService& _loggingService;

public:
    FunctionManager( LoggingService& loggingService );

    bool isFunctionRegistered( int id ) const;

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

    bool removeFunction( int id );

    bool removeFunction( const std::string& name );

    bool invokeFunction( TaskBase& task );

    FunctionResultType invokeReaction( const Ip6Addr& addr, const TaskBase& task );

    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( int functionId );

    std::optional< std::reference_wrapper< FunctionConcept > > getFunction( const std::string& functionName );
};