#include "../tasks/serializable/serializable.hpp"

enum FunctionResultType
{
    SUCCESS,
    FAILURE,
    TRY_AGAIN,
    TRY_AGAIN_LOCAL,
};

template< SerializableOrTrivial Result >
struct FunctionResult
{
    FunctionResult( std::optional< Result > value, FunctionResultType resultType )
    : result( value ), resultType( resultType ) {}
    /// @brief The result value of the function.
    std::optional< Result > result;
    
    /// @brief Denotes whether the function is considered as a success.
    FunctionResultType resultType;

    bool isSuccessful()
    {
        return resultType == FunctionResultType::SUCCESS;
    }

    bool shouldLeaderReschedule()
    {
        return resultType == FunctionResultType::TRY_AGAIN;
    }

    bool shouldFollowerReschedule()
    {
        return resultType == FunctionResultType::TRY_AGAIN_LOCAL;
    }
};