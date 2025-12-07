#include "functionRegistry.hpp"

class FunctionFacade
{
    FunctionRegistry& _functionRegistry;
public:
    FunctionFacade( FunctionRegistry& functionRegistry );

    /// @brief Removes all tasks from all schedulers on this module.
    void clearAllTasks();
    /// @brief Clears all task schedulers for scheduling tasks.
    /// @param hardUnblock Removes active barrier if true, otherwise the barrier remains active.
    void unblockTaskSchedulers( bool hardUnblock = false );

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionName The name of the distributed function.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( const std::string& functionName )
    {
        return _functionRegistry.getFunctionHandle< Result, Arguments... >( functionName );
    }

    /// @brief Retrieves a function handle. The function handle is used for invoking a function over the network.
    /// @tparam Result A trivially copyable type, or a type that implements Serializable. Denotes the type of the function's result. 
    /// @tparam ...Arguments A pack of trivially copyable types, or types that implement Serializable. Denotes the types of the function's parameters.
    /// @param functionId The ID of the distributed function. You may also use the std::string variant for more human-friendly retrieval.
    /// @return std::nullopt if the function does not exist, otherwise the function handle is returned.
    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
    std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( int functionId )
    {
        return _functionRegistry.getFunctionHandle< Result, Arguments... >( functionId );
    }

    template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments,
              std::derived_from< DistributedFunction< Result, Arguments... > > Func >
    bool registerFunction( const Func& function )
    {
        return _functionRegistry.registerFunction< Result, Arguments... >( function );
    }

};