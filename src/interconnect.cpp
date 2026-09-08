#include "interconnect.hpp"

#include <cstdint>

void Interconnect::attach_cache(Cache& cache)
{
    m_caches.push_back(&cache);
}

Interconnect::BroadcastResult Interconnect::broadcast(BusRequest request, std::uint32_t address,  Cache& requester)
{
    BroadcastResult broadcast_result{};
    
    for (Cache* cache : m_caches)
    {
        if (cache != &requester)
        {
            const auto snoop_result{cache->snoop(request, address)};

            // data comes from another cache
            if (snoop_result.hit)
            {
                broadcast_result.data_found = true;
                broadcast_result.data = snoop_result.data;
                return broadcast_result;
            }
        }
    }

    // all caches checked, get data from memory
    return broadcast_result;
}
                   