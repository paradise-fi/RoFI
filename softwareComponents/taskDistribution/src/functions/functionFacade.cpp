#include "../../include/functionFacade.hpp"

FunctionFacade::FunctionFacade( FunctionRegistry& functionRegistry ) : _functionRegistry( functionRegistry ) {}

void FunctionFacade::clearAllTasks()
{
    _functionRegistry.clearAll();
}

/// @brief Clears all task schedulers for scheduling tasks.
/// @param hardUnblock Removes active barrier if true, otherwise the barrier remains active.
void FunctionFacade::unblockTaskSchedulers( bool hardUnblock )
{
    _functionRegistry.unblockTaskSchedulers( hardUnblock );
}