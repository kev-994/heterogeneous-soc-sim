#include "cache.hpp"

#include <cassert>

int main()
{
    Cache cache{};

    // test cache fill
    assert(cache.read(0x00) == Cache::CacheResult::Miss);
    assert(cache.read(0x00) == Cache::CacheResult::Hit);

    // test two-way associativity
    assert(cache.read(0x10) == Cache::CacheResult::Miss);
    assert(cache.read(0x30) == Cache::CacheResult::Miss);
    assert(cache.read(0x10) == Cache::CacheResult::Hit);
    assert(cache.read(0x30) == Cache::CacheResult::Hit);

    // both cache lines in this set are valid
    assert(cache.read(0x50) == Cache::CacheResult::Miss);

    // way 0 replacement
    assert(cache.read(0x50) == Cache::CacheResult::Hit);
    assert(cache.read(0x10) == Cache::CacheResult::Miss);
    assert(cache.read(0x30) == Cache::CacheResult::Hit);
}