#pragma once

#include "bus.hpp"
#include "memory.hpp"
#include "cache.hpp"

#include <cstdint>
#include <vector>

class Cache;

class Interconnect
{
public:
    Interconnect(Memory& memory)
        : m_memory{memory}
    {}

    struct BroadcastResult
    {
        bool data_found{};
        std::array<std::uint8_t, 16> data{};
    };

    void attach_cache(Cache& cache);

    BroadcastResult broadcast(BusRequest request, std::uint32_t address, Cache& requester);

private:
    std::vector<Cache*> m_caches;
    Memory& m_memory;
};