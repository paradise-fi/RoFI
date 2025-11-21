#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "lwip++.hpp"

using namespace rofi::hal;

enum MemoryPropagationType
{
    NONE,
    ONE_TARGET,
    SEND_TO_ALL,
};

struct MemoryWriteResult
{
    // Instructs the system that the memory storage pipeline should continue as normal.
    bool success;

    // Instructs the system that this module did actually store the memory.
    bool stored;

    // Instructs the system to only write metadata.
    bool metadataOnly;

    // Instructs the system on how the data should be propagated.
    MemoryPropagationType propagationType;

    // Used only if the propagation is ONE_TARGET. If null and propagation is ONE_TARGET, this will target the leader.
    std::optional< Ip6Addr > propagationTarget;

    MemoryWriteResult() = default;

    MemoryWriteResult( bool success, bool stored, bool metadataOnly, MemoryPropagationType propagationType, Ip6Addr target )
    : success( success ), stored( stored ), metadataOnly( metadataOnly ), propagationType( propagationType ), propagationTarget( target ) {}

    MemoryWriteResult( bool success, bool stored, bool metadataOnly, MemoryPropagationType propagationType )
    : success( success ), stored( stored ), metadataOnly( metadataOnly ), propagationType( propagationType ) {}
};