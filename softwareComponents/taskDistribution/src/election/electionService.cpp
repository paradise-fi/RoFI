#include "electionService.hpp"

ElectionService::ElectionService( std::unique_ptr< ElectionProtocolBase > election, LoggingService& logger )
: _election( std::move( election ) ), _logger( logger )
{
    _election->registerElectionFailedCallback( [ this ] { onLeaderFailed(); } );
    _election->registerElectionFinishedCallback( [ this ] { onLeaderElected(); } );
}

ElectionService::~ElectionService()
{
    if ( _election )
    {
        _election->unregisterCallbacks();
    }
}

void ElectionService::start( int initialElectionDelay,
    std::function< void( const Ip6Addr& ) >&& electionFinishedCallback,
    int electionCyclesBeforeStabilization )
{
    _electionCyclesBeforeStabilization = electionCyclesBeforeStabilization;
    _onLeaderSuccess = std::forward< std::function< void( const Ip6Addr& ) > >( electionFinishedCallback );
    _election->start( initialElectionDelay );
    _isRunning = true;
}

bool ElectionService::isRunning()
{
    return _isRunning;
}

bool ElectionService::isElectionComplete()
{
    return _electedCount > _electionCyclesBeforeStabilization;
}

const Ip6Addr& ElectionService::getLeader()
{
    return _election->getLeader();
}

bool ElectionService::registerLeaderFailureCallback( std::function< void() >&& callback )
{
    if ( _onLeaderFailed.has_value() )
    {
        _logger.logWarning( "[ElectionService] onLeaderFailure already registered. This call had no effect." );
        return false;
    }

    _onLeaderFailed = std::forward< std::function< void() > >( callback );
    return true;
}

bool ElectionService::unregisterLeaderFailureCallback()
{
    if ( !_onLeaderFailed.has_value() )
    {
        return false;
    }

    _onLeaderFailed.reset();
    return true;
}

// ============================ PRIVATE
void ElectionService::onLeaderElected()
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

void ElectionService::onLeaderFailed()
{
    _logger.logInfo("[ElectionService] Leader failure detected.", LogVerbosity::Medium );
    _electedCount = 0;

    if ( _onLeaderFailed.has_value() )
    {
        _onLeaderFailed.value()();
    }
}