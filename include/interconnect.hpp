#pragma once

#include "cache.hpp"
#include "types.hpp"

#include <vector>

class Interconnect
{
public:
    void attach_cache(Cache& cache);
    void broadcast(const CoherenceTransaction& transaction);

private:
    std::vector<Cache*> m_caches{};
};