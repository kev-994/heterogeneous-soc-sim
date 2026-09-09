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