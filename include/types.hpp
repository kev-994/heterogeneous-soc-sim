#pragma once

#include <cstdint>

using Address = std::uint32_t;
using AgentId = std::uint32_t;
using TransactionId = std::uint64_t;
enum class AgentType
{
    CPU,
    DMA,
    Accelerator
};
enum class MemoryOperation
{
    Read,
    Write
};
enum class SimulationMode
{
    Functional,
    Timing
};
enum class CoherenceTransaction
{
    BusRd,  // Lose exclusive ownership
    BusRdX, // Another cache wants the line and exclusive ownership.
    BusUpgr // The requester already has a shared copy and wants to become the sole owner.
};