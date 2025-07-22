#include "functionModel.hpp"
#include <functional>

template< typename Result, typename... Arguments > 
class FunctionHandle
{
    FunctionModel< Result, Arguments... >& _internal;
    std::function< bool( const Ip6Addr&, int, bool, FunctionModel< Result, Arguments...>& fn, std::tuple< Arguments...> && arguments ) > _executor;

public:
    FunctionHandle( FunctionModel< Result, Arguments... >& internalHandle, 
    std::function< bool( const Ip6Addr&, int, bool, FunctionModel< Result, Arguments...>& fn, std::tuple< Arguments...> && arguments ) > executor)
    : _internal( internalHandle ), _executor( executor ){}

    bool operator()( const Ip6Addr& receiver, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments )
    {
        return _executor( receiver, priority, setTopPriority, _internal, arguments );
    }
};