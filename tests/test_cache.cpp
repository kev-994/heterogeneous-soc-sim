#include "cache.hpp"
#include "interconnect.hpp"
#include "memory.hpp"
#include "bus.hpp"

#include <cassert>

int main()
{
    // ---------------------------------------------------------
    // 1. Read miss followed by read hit
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache{memory};

        const auto first = cache.read(0x00);

        assert(first.result == Cache::CacheResult::Miss);
        assert(first.data == memory.get_byte(0x00));

        const auto second = cache.read(0x00);

        assert(second.result == Cache::CacheResult::Hit);
        assert(second.data == memory.get_byte(0x00));
    }

    // ---------------------------------------------------------
    // 2. Read should return actual data
    //
    // Set up memory BEFORE accessing the address so that the
    // value is present when the cache line is first installed.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache{memory};

        memory.write(0x18, 42);

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
        Memory memory{};
        Cache cache{memory};

        memory.write(0x18, 42);
        memory.write(0x1C, 99);

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
        Memory memory{};
        Cache cache{memory};

        memory.write(0x24, 10);

        const auto result = cache.read(0x24);

        assert(result.result == Cache::CacheResult::Miss);
        assert(result.data == 10);

        cache.write(0x24, 55);

        const auto after_write = cache.read(0x24);

        assert(after_write.result == Cache::CacheResult::Hit);
        assert(after_write.data == 55);

        // Write-back means memory still contains the old value.
        assert(memory.get_byte(0x24) == 10);

        assert(cache.get_state(0x24) == MESIState::Modified);
    }

    // ---------------------------------------------------------
    // 5. Write miss into an empty way
    //
    // Writing to an uncached address should:
    //
    //     1. Load the entire cache line from memory.
    //     2. Modify the requested byte.
    //     3. Mark the line Modified.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache{memory};

        memory.write(0x30, 30);

        cache.write(0x30, 77);

        const auto result = cache.read(0x30);

        assert(result.result == Cache::CacheResult::Hit);
        assert(result.data == 77);

        // Write-back means backing memory has not changed yet.
        assert(memory.get_byte(0x30) == 30);

        assert(cache.get_state(0x30) == MESIState::Modified);
    }

    // ---------------------------------------------------------
    // 6. Two-way associativity
    //
    // 0x10 and 0x30 map to the same set but have different tags.
    // A 2-way cache should allow both to coexist.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache{memory};

        memory.write(0x10, 10);
        memory.write(0x30, 30);

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
        Memory memory{};
        Cache cache{memory};

        memory.write(0x10, 10);
        memory.write(0x30, 30);
        memory.write(0x50, 50);

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
    // We modify 0x10, making its cache line Modified.
    //
    // Accessing 0x50 forces way 0 to be evicted.
    // The dirty 0x10 line must therefore be written back
    // to shared memory before replacement.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache{memory};

        memory.write(0x10, 10);
        memory.write(0x30, 30);
        memory.write(0x50, 50);

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

        assert(cache.get_state(0x10) == MESIState::Modified);

        // Write-back means memory still contains the old value.
        assert(memory.get_byte(0x10) == 10);

        // 0x50 maps to the same set, forcing way 0 eviction.
        const auto third = cache.read(0x50);

        assert(third.result == Cache::CacheResult::Miss);
        assert(third.data == 50);

        // The dirty 0x10 line should have been written back.
        assert(memory.get_byte(0x10) == 99);

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

    // ---------------------------------------------------------
    // 9. Exclusive -> Shared snoop transition
    //
    // Cache 0 initially owns the line in E state.
    // Cache 1 issues BusRd.
    //
    // Cache 0 should transition:
    //
    //     E -> S
    //
    // Cache 1 will receive the line separately through the
    // interconnect data-transfer mechanism.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        cache0.read(0x00);

        assert(cache0.get_state(0x00) == MESIState::Exclusive);

        // Cache 1 wants to read 0x00.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        assert(cache0.get_state(0x00) == MESIState::Shared);

        // Cache 1 should now have the line.
        assert(cache1.get_state(0x00) == MESIState::Shared);

        const auto result = cache1.read(0x00);

        assert(result.result == Cache::CacheResult::Hit);
    }

    // ---------------------------------------------------------
    // 10. Shared -> Shared snoop
    //
    // Once a line is Shared, another BusRd should leave the
    // existing cache in Shared state.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        cache0.read(0x00);

        assert(cache0.get_state(0x00) == MESIState::Exclusive);

        // First reader creates the shared state.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        assert(cache0.get_state(0x00) == MESIState::Shared);
        assert(cache1.get_state(0x00) == MESIState::Shared);

        // Another BusRd should not change either cache from S.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        assert(cache0.get_state(0x00) == MESIState::Shared);
        assert(cache1.get_state(0x00) == MESIState::Shared);
    }

    // ---------------------------------------------------------
    // 11. Invalid -> Invalid snoop
    //
    // A cache that does not contain the requested line should
    // remain Invalid when it observes a BusRd.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        assert(cache0.get_state(0x00) == MESIState::Invalid);

        // Cache 1 wants to read 0x00.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        assert(cache0.get_state(0x00) == MESIState::Invalid);
    }

    // ---------------------------------------------------------
    // 12. Modified -> Shared snoop
    //
    // Cache 0 owns a modified copy.
    //
    // Cache 1 requests the line using BusRd.
    //
    // Cache 0 must:
    //
    //     M -> S
    //
    // and supply the modified data.
    //
    // The modified data must also be written back to shared
    // memory because memory may contain stale data while the
    // line is Modified.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        // Make the line Modified in cache 0.
        cache0.write(0x00, 42);

        assert(cache0.get_state(0x00) == MESIState::Modified);

        // Memory should still contain the old value because the
        // cache uses write-back.
        assert(memory.get_byte(0x00) == 0);

        const auto snoop_result =
            cache0.snoop(BusRequest::BusRd, 0x00);

        assert(cache0.get_state(0x00) == MESIState::Shared);

        assert(snoop_result.has_data == true);
        assert(snoop_result.data[0] == 42);

        // M -> S requires the modified data to become visible
        // in shared memory.
        assert(memory.get_byte(0x00) == 42);
    }

    // ---------------------------------------------------------
    // 13. End-to-end Modified -> Shared data transfer
    //
    // This tests the complete path:
    //
    //     Cache 0: M
    //          |
    //       BusRd
    //          |
    //          v
    //     Cache 0: M -> S
    //          |
    //      supplies data
    //          |
    //          v
    //     Cache 1: I -> S
    //
    // The important property is that Cache 1 must receive the
    // value written by Cache 0, even though shared memory was
    // stale before the BusRd.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        // Cache 0 obtains the line and modifies it.
        cache0.write(0x00, 42);

        assert(cache0.get_state(0x00) == MESIState::Modified);
        assert(cache1.get_state(0x00) == MESIState::Invalid);

        // Memory is stale while cache 0 owns the line in M.
        assert(memory.get_byte(0x00) == 0);

        // Cache 1 requests the line.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        // Cache 0 supplies the modified line and becomes Shared.
        assert(cache0.get_state(0x00) == MESIState::Shared);

        // Cache 1 should now contain a Shared copy.
        assert(cache1.get_state(0x00) == MESIState::Shared);

        // The modified value must have been transferred.
        const auto result = cache1.read(0x00);

        assert(result.result == Cache::CacheResult::Hit);
        assert(result.data == 42);

        // Memory should now also contain the current value.
        assert(memory.get_byte(0x00) == 42);
    }

    // ---------------------------------------------------------
    // 14. Interconnect broadcasts BusRd to other caches
    //
    // Verify that the interconnect actually invokes snooping
    // on another cache and causes an M -> S transition.
    // ---------------------------------------------------------
    {
        Memory memory{};
        Cache cache0{memory};
        Cache cache1{memory};

        Interconnect interconnect{};
        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        // Modify this cache line in cache 0.
        cache0.write(0x00, 42);

        assert(cache0.get_state(0x00) == MESIState::Modified);

        // Cache 1 wants to read 0x00.
        interconnect.broadcast(BusRequest::BusRd, 0x00, cache1);

        assert(cache0.get_state(0x00) == MESIState::Shared);
        assert(cache1.get_state(0x00) == MESIState::Shared);

        // Verify the data supplied by cache 0 reached cache 1.
        const auto result = cache1.read(0x00);

        assert(result.result == Cache::CacheResult::Hit);
        assert(result.data == 42);
    }
}

