#include "functionManager.hpp"

FunctionManager::FunctionManager( LoggingService& loggingService ) : _loggingService( loggingService ) {}

bool FunctionManager::isFunctionRegistered( int id ) const
{
    return _functions.find( id ) != _functions.end();
}

bool FunctionManager::removeFunction( int id )
{
    auto fnCandidate = _functions.find( id );

    if ( fnCandidate == _functions.end() )
    {
        return false;
    }

    _nameToIdMap.erase( fnCandidate->second->functionName() );

    return _functions.erase( id ) != 0;
}

bool FunctionManager::removeFunction( const std::string& name )
{
    auto idCandidate = _nameToIdMap.find( name );
    
    if ( idCandidate == _nameToIdMap.end() )
    {
        return false;
    }

    return removeFunction( idCandidate->second );
}

bool FunctionManager::invokeFunction( TaskBase& task )
{
    const auto& fn = _functions.find( task.functionId() );
    
    if ( fn == _functions.end() )
    {
        std::ostringstream stream;
        stream << "Function " << task.functionId() << " not found during function invocation.";
        _loggingService.logWarning( stream.str() );
        task.setStatus( TaskStatus::Failed );
        return false;
    }

    fn->second->perform( task );
    return true;
}

FunctionResultType FunctionManager::invokeReaction( const Ip6Addr& addr, const TaskBase& task )
{
    const auto& fn = _functions.find( task.functionId() );
    if ( fn == _functions.end() )
    {
        std::ostringstream stream;
        stream << "Function " << task.functionId() << " not found when trying to react to module task completion.";
        _loggingService.logError( stream.str() );
        return FunctionResultType::FAILURE;
    }

    if ( task.status() != TaskStatus::Complete )
    {
        return fn->second->onFailure( addr, task ) ? FunctionResultType::TRY_AGAIN_LOCAL : FunctionResultType::SUCCESS;
    }
    
    return fn->second->onSuccess( addr, task ) ? FunctionResultType::TRY_AGAIN_LOCAL : FunctionResultType::SUCCESS;
}

std::optional< std::reference_wrapper< FunctionConcept > > FunctionManager::getFunction( int functionId )
{
    auto kv = _functions.find( functionId );
    
    if ( kv == _functions.end() )
    {
        return std::nullopt;
    }

    return *( kv->second.get() );
}

std::optional< std::reference_wrapper< FunctionConcept > > FunctionManager::getFunction( const std::string& functionName )
{
    auto idCandidate = _nameToIdMap.find( functionName );
    
    if ( idCandidate == _nameToIdMap.end() )
    {
        return std::nullopt;
    }

    return getFunction( idCandidate->second );
}