#include "electionProtocolBase.hpp"
#include "memoryService.hpp"

using namespace rofi::net;
using namespace rofi::leadership;


class ElectionService
{
    std::unique_ptr< ElectionProtocolBase > _election;
    std::function< void( const Ip6Addr& ) > _onLeaderSuccess;
    std::optional< std::function< void() > > _onLeaderFailed;
    int _electedCount = 0;
    bool _isRunning = false;

    void onLeaderElected()
    {
        if ( _electedCount == 3 )
        {
            _onLeaderSuccess( _election->getLeader() );
            _electedCount++;
        }
        if (_electedCount < 3)
        {
            _electedCount++;
        }
    }

    void onLeaderFailed()
    {
        _electedCount = 0;

        if ( _onLeaderFailed.has_value() )
        {
            _onLeaderFailed.value()();
        }
    }

public:
    ElectionService( std::unique_ptr< ElectionProtocolBase > election )
    : _election( std::move( election ) )
    {
        _election->registerElectionFailedCallback( [ this ] { onLeaderFailed(); } );
        _election->registerElectionFinishedCallback( [ this ] { onLeaderElected(); } );
    }

    void start( int moduleId, std::function< void( const Ip6Addr& ) >&& electionFinishedCallback )
    {
        _onLeaderSuccess = std::forward< std::function< void( const Ip6Addr& ) > >( electionFinishedCallback );
        _election->start( moduleId );
        _isRunning = true;
    }

    bool isRunning()
    {
        return _isRunning;
    }

    bool isElectionComplete()
    {
        return _electedCount > 3;
    }

    const Ip6Addr& getLeader()
    {
        return _election->getLeader();
    }

    bool registerLeaderFailureCallback( std::function< void() >&& callback )
    {
        if ( _onLeaderFailed.has_value() )
        {
            return false;
        }

        _onLeaderFailed = std::forward< std::function< void() > >( callback );
    }

    bool unregisterLeaderFailureCallback()
    {
        if ( !_onLeaderFailed.has_value() )
        {
            return false;
        }

        _onLeaderFailed.reset();
        return true;
    }
};