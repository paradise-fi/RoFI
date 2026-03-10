#pragma once
#include "electionProtocolBase.hpp"
#include "../logger/loggingService.hpp"

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

    LoggingService& _logger;

    void onLeaderElected();

    void onLeaderFailed();

public:
    ElectionService( std::unique_ptr< ElectionProtocolBase > election, LoggingService& logger );

    ~ElectionService();

    void start( int initialElectionDelay, std::function< void( const Ip6Addr& ) >&& electionFinishedCallback, int electionCyclesBeforeStabilization = 3 );

    bool isRunning();

    bool isElectionComplete();

    const Ip6Addr& getLeader();

    bool registerLeaderFailureCallback( std::function< void() >&& callback );

    bool unregisterLeaderFailureCallback();
};