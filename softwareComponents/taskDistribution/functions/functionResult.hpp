#include "../tasks/serializable/serializable.hpp"

template< SerializableOrTrivial Result >
struct FunctionResult
{
    FunctionResult( std::optional< Result > value, bool isSuccessful )
    : result( value ), success( isSuccessful ) {}
    /// @brief The result of the function.
    std::optional< Result > result;
    
    /// @brief Denotes whether the function is considered as a success.
    bool success;
};