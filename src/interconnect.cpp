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
    std::array<std::uint8_t, 16> memory_data{};
    std::uint32_t line_base{address & ~0xF};

    for (std::size_t i{}; i < memory_data.size(); ++i)
    {
        memory_data[i] = m_memory.get_byte(line_base+i);
    }
    broadcast_result.data = memory_data;
    
    return broadcast_result;
}
                   