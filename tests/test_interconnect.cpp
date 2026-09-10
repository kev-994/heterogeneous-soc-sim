#include "interconnect.hpp"
#include "cache.hpp"
#include "config.hpp"
#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

void test_requester_is_not_snooped()
{
    SystemConfig config{};

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
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // The requester must not snoop its own transaction.
    // Therefore it must not transition E -> S.
    assert(!response.copy_exists);
    assert(!response.data.has_value());

    const CacheLine* cache0_line{cache0.find_line(0x00)};

    assert(cache0_line != nullptr);
    assert(cache0_line->state == MESIState::Exclusive);
}

void test_other_cache_is_snooped()
{
    SystemConfig config{};

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
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // Another cache has the line.
    assert(response.copy_exists);

    // An Exclusive cache has a clean copy, so it does not supply data.
    assert(!response.data.has_value());

    // cache1 should observe BusRd and transition E -> S.
    const CacheLine* cache1_line{cache1.find_line(0x00)};

    assert(cache1_line != nullptr);
    assert(cache1_line->state == MESIState::Shared);
}

void test_no_other_cache_has_copy()
{
    SystemConfig config{};

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

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // Neither cache contains the line.
    assert(!response.copy_exists);
    assert(!response.data.has_value());

    // Broadcasting must not create a line.
    assert(!cache0.contains(0x00));
    assert(!cache1.contains(0x00));
}

void test_shared_copy_exists_but_does_not_supply_data()
{
    SystemConfig config{};

    Cache cache0{config, 0};
    Cache cache1{config, 1};

    Interconnect interconnect{};

    interconnect.attach_cache(cache0);
    interconnect.attach_cache(cache1);

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.state = MESIState::Shared;

    cache1.install_line(0x00, line, MESIState::Shared);

    CoherenceTransaction transaction{
        1,
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // Another cache has a valid copy.
    assert(response.copy_exists);

    // Shared is clean, so it does not supply data.
    assert(!response.data.has_value());

    // Shared remains Shared.
    const CacheLine* cache1_line{cache1.find_line(0x00)};

    assert(cache1_line != nullptr);
    assert(cache1_line->state == MESIState::Shared);
}

void test_modified_copy_supplies_data()
{
    SystemConfig config{};

    Cache cache0{config, 0};
    Cache cache1{config, 1};

    Interconnect interconnect{};

    interconnect.attach_cache(cache0);
    interconnect.attach_cache(cache1);

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.state = MESIState::Modified;

    // Give the Modified cache a recognisable line.
    for (std::size_t i{}; i < line.data.size(); ++i)
    {
        line.data[i] = static_cast<std::uint8_t>(i + 1);
    }

    cache1.install_line(0x00, line, MESIState::Modified);

    CoherenceTransaction transaction{
        1,
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // Another cache has the line.
    assert(response.copy_exists);

    // Modified cache must supply the newest copy.
    assert(response.data.has_value());
    assert(response.data->data.size() == config.cache_line_size);

    for (std::size_t i{}; i < config.cache_line_size; ++i)
    {
        assert(response.data->data[i] ==
               static_cast<std::uint8_t>(i + 1));
    }

    // Supplier transitions M -> S.
    const CacheLine* cache1_line{cache1.find_line(0x00)};

    assert(cache1_line != nullptr);
    assert(cache1_line->state == MESIState::Shared);
}

void test_multiple_shared_copies()
{
    SystemConfig config{};

    Cache cache0{config, 0};
    Cache cache1{config, 1};
    Cache cache2{config, 2};

    Interconnect interconnect{};

    interconnect.attach_cache(cache0);
    interconnect.attach_cache(cache1);
    interconnect.attach_cache(cache2);

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.state = MESIState::Shared;

    cache1.install_line(0x00, line, MESIState::Shared);
    cache2.install_line(0x00, line, MESIState::Shared);

    CoherenceTransaction transaction{
        1,
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // At least one other cache has the line.
    assert(response.copy_exists);

    // Shared copies are clean, so nobody supplies data.
    assert(!response.data.has_value());

    const CacheLine* cache1_line{cache1.find_line(0x00)};
    const CacheLine* cache2_line{cache2.find_line(0x00)};

    assert(cache1_line != nullptr);
    assert(cache2_line != nullptr);

    assert(cache1_line->state == MESIState::Shared);
    assert(cache2_line->state == MESIState::Shared);
}

void test_requester_modified_copy_is_not_supplier()
{
    SystemConfig config{};

    Cache cache0{config, 0};
    Cache cache1{config, 1};

    Interconnect interconnect{};

    interconnect.attach_cache(cache0);
    interconnect.attach_cache(cache1);

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.state = MESIState::Modified;

    for (std::size_t i{}; i < line.data.size(); ++i)
    {
        line.data[i] = static_cast<std::uint8_t>(0xA0 + i);
    }

    cache0.install_line(0x00, line, MESIState::Modified);

    CoherenceTransaction transaction{
        1,
        0,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // The requester must be skipped even though it has M.
    assert(!response.copy_exists);
    assert(!response.data.has_value());

    // Its state must remain Modified because it was never snooped.
    const CacheLine* cache0_line{cache0.find_line(0x00)};

    assert(cache0_line != nullptr);
    assert(cache0_line->state == MESIState::Modified);
}

void test_busupgr_invalidates_other_shared_copies()
{
    SystemConfig config{};

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
        0,
        Transaction::BusUpgr,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // BusUpgr does not transfer data.
    // The other cache does have a copy, however.
    assert(response.copy_exists);
    assert(!response.data.has_value());

    // The other cache must invalidate its shared copy.
    const CacheLine* cache1_line{cache1.find_line(0x00)};

    assert(cache1_line == nullptr);

    // Requester must not snoop its own BusUpgr.
    const CacheLine* cache0_line{cache0.find_line(0x00)};

    assert(cache0_line != nullptr);
    assert(cache0_line->state == MESIState::Shared);
}

void test_agent_ids_identify_requester()
{
    SystemConfig config{};

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
        42,
        Transaction::BusRd,
        0x00
    };

    const CoherenceResponse response{interconnect.broadcast(transaction)};

    // Agent ID 42 identifies cache0 as the requester.
    assert(!response.copy_exists);
    assert(!response.data.has_value());

    const CacheLine* cache0_line{cache0.find_line(0x00)};

    assert(cache0_line != nullptr);
    assert(cache0_line->state == MESIState::Exclusive);
}

int main()
{
    test_requester_is_not_snooped();
    test_other_cache_is_snooped();
    test_no_other_cache_has_copy();
    test_shared_copy_exists_but_does_not_supply_data();
    test_modified_copy_supplies_data();
    test_multiple_shared_copies();
    test_requester_modified_copy_is_not_supplier();
    test_busupgr_invalidates_other_shared_copies();
    test_agent_ids_identify_requester();

    std::cout << "All interconnect tests passed.\n";

    return 0;
}