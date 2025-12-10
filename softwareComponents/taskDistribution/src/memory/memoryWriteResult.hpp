#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include "lwip++.hpp"
#include "memoryPropagationType.hpp"

using namespace rofi::hal;

struct MemoryWriteResult
{
    // Instructs the system that the memory storage pipeline should continue as normal.
    bool success = false;

    // Instructs the system to only write metadata.
    bool metadataOnly = false;

    // Instructs the system on how the data should be propagated.
    MemoryPropagationType propagationType;

    // Used only if the propagation is ONE_TARGET. If null and propagation is ONE_TARGET, this will target the leader.
    std::optional< Ip6Addr > propagationTarget;

    MemoryWriteResult() = default;

    MemoryWriteResult( bool success,bool metadataOnly, MemoryPropagationType propagationType, Ip6Addr target )
    : success( success ), metadataOnly( metadataOnly ), propagationType( propagationType ), propagationTarget( target ) {}

    MemoryWriteResult( bool success, bool metadataOnly, MemoryPropagationType propagationType )
    : success( success ), metadataOnly( metadataOnly ), propagationType( propagationType ) {}
};