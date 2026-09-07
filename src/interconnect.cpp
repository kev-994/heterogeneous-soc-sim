#include "interconnect.hpp"
#include "cache.hpp"

void Interconnect::attach_cache(Cache& cache)
{
    m_caches.push_back(&cache);
}

void Interconnect::broadcast(BusRequest request, std::uint32_t address,  Cache& requester)
{
    for (Cache* cache : m_caches)
    {
        if (cache != &requester)
        {
            const auto snoop_result{cache->snoop(request, address)};

            // transfer data if necessary
            if (snoop_result.has_data)
            {
            
            }
        }
    }
}
                   