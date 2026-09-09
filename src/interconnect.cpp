#include "interconnect.hpp"

#include "cache.hpp"
#include "types.hpp"

#include <vector>

void Interconnect::attach_cache(Cache& cache)
{
    m_caches.push_back(&cache);
}

void Interconnect::broadcast(const CoherenceTransaction& transaction)
{
    for (Cache* cache : m_caches)
    {
        if (cache->agent_id() == transaction.agent_id) // skip requester
        {
            continue;
        }

        cache->snoop(transaction.type, transaction.address);
    }
}