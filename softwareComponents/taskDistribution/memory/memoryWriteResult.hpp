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
    bool success;

    // Instructs the system to only write metadata.
    bool metadataOnly;

    // Instructs the system on how the data should be propagated.
    MemoryPropagationType propagationType;

    // Used only if the propagation is ONE_TARGET.
    std::optional< Ip6Addr > propagationTarget;

    MemoryWriteResult() = default;

    MemoryWriteResult( bool success, bool metadataOnly, MemoryPropagationType propagationType, Ip6Addr target )
    : success( success ), metadataOnly( metadataOnly ), propagationType( propagationType ), propagationTarget( target ) {}

    MemoryWriteResult( bool success, bool metadataOnly, MemoryPropagationType propagationType )
    : success( success ), metadataOnly( metadataOnly ), propagationType( propagationType ) {}
};