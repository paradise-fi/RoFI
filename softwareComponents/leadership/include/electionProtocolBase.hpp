#pragma once

#include <iostream>
#include <networking/networkManager.hpp>
#include <LRHelper.hpp>
#include <networking/protocols/messageDistributor.hpp>
#include <set>
#include <thread>
#include <mutex>
#include <functional>

namespace rofi::leadership {
    using namespace rofi::net;

    /// @brief A base class for a wrapper that interacts with an election protocol on the network layer.
    class ElectionProtocolBase
    {
    public:
        virtual ~ElectionProtocolBase() = default;

        /// @brief Used by services to register callbacks for when an election round is finished.
        /// @param cb - a callback function that will be invoked by the ElectionProtocolBase instance on election round finish.
        virtual void registerElectionFinishedCallback( std::function< void() > cb ) = 0;
        
        /// @brief Used by services to register callbacks for when a leader fails and a new election must start.
        /// @param cb - a callback function that will be invoked by the ElectionProtocolBase instance on leader failure.
        virtual void registerElectionFailedCallback( std::function< void() > cb ) = 0;

        /// @brief Retrieves the address of the current leader.
        /// @return IP address of the leader.
        virtual const Ip6Addr& getLeader() = 0;

        /// @brief Starts up the protocol, or does initialization for it.
        /// @param value TODO: Maybe consider changing this?
        virtual void start( int value ) = 0;
    };
}