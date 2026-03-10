#pragma once

/// @brief Determines how the memory storage is to be handled.
/// If leader first, local memory is not touched at all unless messaged externally.
/// If local first, the memory service first attempts to store in the local memory and then sends details to the leader.
/// Leader contact can be ommited by setting the MemoryPropagationType in the MemoryWriteResult structure.
enum class MemoryStorageBehavior
{
    LocalFirstStorage,
    LeaderFirstStorage,
};