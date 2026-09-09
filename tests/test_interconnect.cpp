#include "interconnect.hpp"
#include "cache.hpp"
#include "config.hpp"
#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    SystemConfig config{};

    // ------------------------------------------------------------
    // Test 1: Requester is not snooped
    // ------------------------------------------------------------

    {
        Cache cache0{config, 0};
        Cache cache1{config, 1};

        Interconnect interconnect{};

        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        CacheLine line{};
        line.data.resize(config.cache_line_size);
        line.state = MESIState::Exclusive;

        cache0.install_line(0x00, line, MESIState::Exclusive);

        CoherenceTransaction transaction{
            1,
            0,                  // requester = cache0
            Transaction::BusRd,
            0x00
        };

        interconnect.broadcast(transaction);

        // The requester must not snoop its own transaction.
        // If it did, E -> S.
        const CacheLine* cache0_line{cache0.find_line(0x00)};

        assert(cache0_line != nullptr);
        assert(cache0_line->state == MESIState::Exclusive);
    }

    // ------------------------------------------------------------
    // Test 2: Other cache is snooped
    // ------------------------------------------------------------

    {
        Cache cache0{config, 0};
        Cache cache1{config, 1};

        Interconnect interconnect{};

        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        CacheLine line{};
        line.data.resize(config.cache_line_size);
        line.state = MESIState::Exclusive;

        cache1.install_line(0x00, line, MESIState::Exclusive);

        CoherenceTransaction transaction{
            1,
            0,                  // requester = cache0
            Transaction::BusRd,
            0x00
        };

        interconnect.broadcast(transaction);

        // cache1 should observe the BusRd and transition E -> S.
        const CacheLine* cache1_line{cache1.find_line(0x00)};

        assert(cache1_line != nullptr);
        assert(cache1_line->state == MESIState::Shared);
    }

    // ------------------------------------------------------------
    // Test 3: BusUpgr invalidates other shared copies
    // ------------------------------------------------------------

    {
        Cache cache0{config, 0};
        Cache cache1{config, 1};

        Interconnect interconnect{};

        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        CacheLine line{};
        line.data.resize(config.cache_line_size);
        line.state = MESIState::Shared;

        cache0.install_line(0x00, line, MESIState::Shared);
        cache1.install_line(0x00, line, MESIState::Shared);

        CoherenceTransaction transaction{
            1,
            0,                  // requester = cache0
            Transaction::BusUpgr,
            0x00
        };

        interconnect.broadcast(transaction);

        // The other cache must invalidate its shared copy.
        const CacheLine* cache1_line{cache1.find_line(0x00)};

        assert(cache1_line == nullptr);

        // The requester must not snoop its own BusUpgr.
        // Therefore its copy should remain Shared for now.
        const CacheLine* cache0_line{cache0.find_line(0x00)};

        assert(cache0_line != nullptr);
        assert(cache0_line->state == MESIState::Shared);
    }

    // ------------------------------------------------------------
    // Test 4: Broadcasting an address absent from other caches
    // ------------------------------------------------------------

    {
        Cache cache0{config, 0};
        Cache cache1{config, 1};

        Interconnect interconnect{};

        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        CoherenceTransaction transaction{
            1,
            0,
            Transaction::BusRd,
            0x00
        };

        // Neither cache contains the line.
        // The broadcast should simply do nothing to either cache
        // and, importantly, should not crash.
        interconnect.broadcast(transaction);

        assert(!cache0.contains(0x00));
        assert(!cache1.contains(0x00));
    }

    // ------------------------------------------------------------
    // Test 5: Agent IDs identify the requester
    // ------------------------------------------------------------

    {
        Cache cache0{config, 42};
        Cache cache1{config, 99};

        assert(cache0.agent_id() == 42);
        assert(cache1.agent_id() == 99);

        Interconnect interconnect{};

        interconnect.attach_cache(cache0);
        interconnect.attach_cache(cache1);

        CacheLine line{};
        line.data.resize(config.cache_line_size);
        line.state = MESIState::Exclusive;

        cache0.install_line(0x00, line, MESIState::Exclusive);

        CoherenceTransaction transaction{
            1,
            42,                 // requester = cache0
            Transaction::BusRd,
            0x00
        };

        interconnect.broadcast(transaction);

        // Because agent_id == 42, cache0 is correctly identified
        // as the requester and therefore does not snoop itself.
        const CacheLine* cache0_line{cache0.find_line(0x00)};

        assert(cache0_line != nullptr);
        assert(cache0_line->state == MESIState::Exclusive);
    }

    std::cout << "All interconnect tests passed.\n";

    return 0;
}
