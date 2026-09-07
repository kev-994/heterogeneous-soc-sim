#pragma once

#include "bus.hpp"

#include <cstdint>
#include <vector>

class Cache;

class Interconnect
{
public:
    void attach_cache(Cache& cache);

    void broadcast(BusRequest request,
                   std::uint32_t address,
                   Cache& requester);

private:
    std::vector<Cache*> m_caches;
};