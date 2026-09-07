#include "cache.hpp"
#include "interconnect.hpp"

#include <cassert>

int main()
{
    // ---------------------------------------------------------
    // 1. Read miss followed by read hit
    // ---------------------------------------------------------
    {
        Cache cache{};

        const auto first = cache.read(0x00);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == getByte(cache, 0x00));

        const auto second = cache.read(0x00);

        assert(second.result == Cache::CacheResult::Hit);
        assert(second.data == getByte(cache, 0x00));
    }

    // ---------------------------------------------------------
    // 2. Read should return actual data
    //
    // Set up memory BEFORE accessing the address so that the
    // value is present when the cache line is first installed.
    // ---------------------------------------------------------
    {
        Cache cache{};

        cache.write_memory(0x18, 42);

        const auto first = cache.read(0x18);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == 42);

        const auto second = cache.read(0x18);

        assert(second.result == Cache::CacheResult::Hit);
        assert(second.data == 42);
    }

    // ---------------------------------------------------------
    // 3. Spatial locality
    //
    // 0x18 and 0x1C are in the same 16-byte cache line:
    //
    //     0x10 ---------------- 0x1F
    //             0x18   0x1C
    //
    // Reading 0x18 should bring the entire 0x10-0x1F line
    // into the cache. Therefore reading 0x1C should hit.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up memory BEFORE the line is loaded.
        cache.write_memory(0x18, 42);
        cache.write_memory(0x1C, 99);

        const auto first = cache.read(0x18);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == 42);

        const auto second = cache.read(0x1C);

        assert(second.result == Cache::CacheResult::Hit);
        assert(second.data == 99);
    }

    // ---------------------------------------------------------
    // 4. Write hit
    //
    // First read brings 0x20-0x2F into the cache.
    // Then writing 0x24 modifies the cached line.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up memory BEFORE accessing the address.
        cache.write_memory(0x24, 10);

        const auto result = cache.read(0x24);

        assert(result.result == Cache::CacheResult::Miss);
        assert(result.data == 10);

        // Write through the cache.
        cache.write(0x24, 55);

        const auto after_write = cache.read(0x24);

        assert(after_write.result == Cache::CacheResult::Hit);
        assert(after_write.data == 55);

        // Write-back means memory still contains the old value.
        assert(getByte(cache, 0x24) == 10);
    }

    // ---------------------------------------------------------
    // 5. Write miss into an empty way
    //
    // Writing to an uncached address should:
    //
    //     1. Load the entire cache line from memory.
    //     2. Modify the requested byte.
    //     3. Mark the line dirty.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up memory BEFORE the cache line is loaded.
        cache.write_memory(0x30, 30);

        cache.write(0x30, 77);

        const auto result = cache.read(0x30);

        assert(result.result == Cache::CacheResult::Hit);
        assert(result.data == 77);

        // Write-back means backing memory has not changed yet.
        assert(getByte(cache, 0x30) == 30);
    }

    // ---------------------------------------------------------
    // 6. Two-way associativity
    //
    // 0x10 and 0x30 map to the same set but have different tags.
    // A 2-way cache should allow both to coexist.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up memory BEFORE accessing the addresses.
        cache.write_memory(0x10, 10);
        cache.write_memory(0x30, 30);

        const auto first = cache.read(0x10);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == 10);

        const auto second = cache.read(0x30);

        assert(second.result == Cache::CacheResult::Miss);
        assert(second.data == 30);

        // Both should still be present.
        const auto third = cache.read(0x10);

        assert(third.result == Cache::CacheResult::Hit);
        assert(third.data == 10);

        const auto fourth = cache.read(0x30);

        assert(fourth.result == Cache::CacheResult::Hit);
        assert(fourth.data == 30);
    }

    // ---------------------------------------------------------
    // 7. Clean-line eviction
    //
    // 0x10 and 0x30 occupy both ways of set 1.
    //
    // 0x50 also maps to set 1, so way 0 (containing 0x10)
    // is replaced.
    //
    // Since 0x10 is clean, no write-back is necessary.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up all memory BEFORE any relevant cache line
        // is accessed.
        cache.write_memory(0x10, 10);
        cache.write_memory(0x30, 30);
        cache.write_memory(0x50, 50);

        const auto first = cache.read(0x10);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == 10);

        const auto second = cache.read(0x30);

        assert(second.result == Cache::CacheResult::Miss);
        assert(second.data == 30);

        // 0x50 maps to the same set and forces way 0 eviction.
        const auto third = cache.read(0x50);

        assert(third.result == Cache::CacheResult::Miss);
        assert(third.data == 50);

        // 0x10 should have been evicted.
        const auto fourth = cache.read(0x10);

        assert(fourth.result == Cache::CacheResult::Miss);
        assert(fourth.data == 10);

        // 0x30 occupied way 1 and should still be cached.
        const auto fifth = cache.read(0x30);

        assert(fifth.result == Cache::CacheResult::Hit);
        assert(fifth.data == 30);
    }

    // ---------------------------------------------------------
    // 8. Dirty-line write-back on eviction
    //
    // 0x10 occupies way 0.
    // 0x30 occupies way 1.
    //
    // We modify 0x10, making its cache line dirty.
    //
    // Accessing 0x50 forces way 0 to be evicted.
    // The dirty 0x10 line must therefore be written back
    // to memory before replacement.
    // ---------------------------------------------------------
    {
        Cache cache{};

        // Set up memory BEFORE accessing the addresses.
        cache.write_memory(0x10, 10);
        cache.write_memory(0x30, 30);
        cache.write_memory(0x50, 50);

        // Fill way 0 with 0x10.
        const auto first = cache.read(0x10);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == 10);

        // Fill way 1 with 0x30.
        const auto second = cache.read(0x30);

        assert(second.result == Cache::CacheResult::Miss);
        assert(second.data == 30);

        // Modify 0x10 in the cache.
        cache.write(0x10, 99);

        // Write-back means memory still contains the old value.
        assert(getByte(cache, 0x10) == 10);

        // 0x50 maps to the same set, forcing way 0 eviction.
        const auto third = cache.read(0x50);

        assert(third.result == Cache::CacheResult::Miss);
        assert(third.data == 50);

        // The dirty 0x10 line should have been written back.
        assert(getByte(cache, 0x10) == 99);

        // 0x30 should still be cached.
        const auto fourth = cache.read(0x30);

        assert(fourth.result == Cache::CacheResult::Hit);
        assert(fourth.data == 30);

        // 0x10 was evicted, so reading it again should miss,
        // but retrieve the value that was written back.
        const auto fifth = cache.read(0x10);

        assert(fifth.result == Cache::CacheResult::Miss);
        assert(fifth.data == 99);
    }

    // test exclusive to shared snoop transition
    {
        Cache cache0{};
        Cache cache1{};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        cache0.read(0x00);
        assert(cache0.get_state(0x00) == MESIState::Exclusive);

        // cache 1 wants to read 0x00
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);
        assert(cache0.get_state(0x00) == MESIState::Shared);

        // shared to shared
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);
        assert(cache0.get_state(0x00) == MESIState::Shared);
    }

    // test invalid to invalid snoop 
    {
        Cache cache0{};
        Cache cache1{};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        assert(cache0.get_state(0x00) == MESIState::Invalid);

        // cache 1 wants to read 0x00
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);
        assert(cache0.get_state(0x00) == MESIState::Invalid);
    }

    // test modified to shared snoop
    {
        Cache cache0{};
        Cache cache1{};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        // modify this cache line in cache0
        cache0.write(0x00, 0);

        assert(cache0.get_state(0x00) == MESIState::Modified);

        // cache 1 wants to read 0x00
        const auto snoop_result{cache0.snoop(BusRequest::BusRd, 0x00)};
        assert(cache0.get_state(0x00) == MESIState::Shared);
        assert(snoop_result.has_data == true);
    }

    // then test the interconnect
    {
        Cache cache0{};
        Cache cache1{};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        // modify this cache line in cache0
        cache0.write(0x00, 0);

        assert(cache0.get_state(0x00) == MESIState::Modified);

        // cache 1 wants to read 0x00
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);
        assert(cache0.get_state(0x00) == MESIState::Shared);
    }

}