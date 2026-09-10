#pragma once

#include "cache.hpp"
#include "types.hpp"
#include "memory.hpp"

#include <vector>
#include <optional>
#include <cstdint>
#include <cassert>

struct CoherenceResponse
{
    std::optional<LineData> data{};
    bool copy_exists{};
};

class Interconnect
{
public:
    void attach_cache(Cache& cache);
    CoherenceResponse broadcast(CoherenceTransaction& transaction);

private:
    std::vector<Cache*> m_caches{};
};