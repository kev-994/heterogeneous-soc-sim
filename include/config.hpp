#pragma once

#include "types.hpp"

#include <cstdint>

struct SystemConfig
{
    // Address/memory
    std::uint32_t address_width{32}; // bits
    std::uint32_t memory_capacity{256}; // bytes

    // Agents
    std::uint32_t cpu_count{2};
    std::uint32_t dma_count{1};
    std::uint32_t accelerator_count{1};

    // Cache geometry
    std::uint32_t cache_line_size{16}; // bytes
    std::uint32_t cache_set_count{2};
    std::uint32_t cache_associativity{2}; // way associativity

    // Simulation
    SimulationMode simulation_mode{SimulationMode::Functional};
};
