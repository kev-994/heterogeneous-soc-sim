#include "interconnect.hpp"

void Interconnect::attach_cache(Cache& cache)
{
    m_caches.push_back(&cache);
}

CoherenceResponse Interconnect::broadcast(CoherenceTransaction& transaction)
{
    std::uint32_t suppliers{};
    CoherenceResponse response{};
    
    for (Cache* cache : m_caches)
    {
        if (cache->agent_id() == transaction.agent_id) // skip requester
        {
            continue;
        }

        const auto result{cache->snoop(transaction.type, transaction.address)};

        if (result.copy_exists)
        {
            response.copy_exists = true;
        }

        if (result.supplies_data)
        {
            response.data = result.line_data; // take data from cache that supplies it
            ++suppliers;
        }
    }

    assert(suppliers <= 1);

    return response;
}