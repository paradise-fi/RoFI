#pragma once
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
    int _electionCyclesBeforeStabilization = 3;

    void onLeaderElected()
    {
        if ( _electedCount == _electionCyclesBeforeStabilization )
        {
            _onLeaderSuccess( _election->getLeader() );
            _electedCount++;
        }

        if (_electedCount < _electionCyclesBeforeStabilization )
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

    void start( int initialElectionDelay, std::function< void( const Ip6Addr& ) >&& electionFinishedCallback, int electionCyclesBeforeStabilization = 3 )
    {
        _electionCyclesBeforeStabilization = electionCyclesBeforeStabilization;
        _onLeaderSuccess = std::forward< std::function< void( const Ip6Addr& ) > >( electionFinishedCallback );
        _election->start( initialElectionDelay );
        _isRunning = true;
    }

    bool isRunning()
    {
        return _isRunning;
    }

    bool isElectionComplete()
    {
        return _electedCount > _electionCyclesBeforeStabilization;
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
        return true;
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