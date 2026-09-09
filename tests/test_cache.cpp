#include "cache.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

void test_address_zero()
{
    SystemConfig config{};
    Cache cache{config, 0};

    AddressDecomposition result = cache.decompose(0);

    assert(result.offset == 0);
    assert(result.set_index == 0);
    assert(result.tag == 0);
}

void test_offset_decomposition()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // 16-byte cache line -> 4 offset bits.
    AddressDecomposition result = cache.decompose(0x0F);

    assert(result.offset == 0x0F);
    assert(result.set_index == 0);
    assert(result.tag == 0);
}

void test_offset_rollover()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // Address 0x10 is the start of the next cache line.
    AddressDecomposition result = cache.decompose(0x10);

    assert(result.offset == 0);
    assert(result.set_index == 1);
    assert(result.tag == 0);
}

void test_set_decomposition()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // Current configuration:
    // 16-byte lines -> 4 offset bits
    // 2 sets        -> 1 set bit
    //
    // 0x10 = 0001 0000
    //              ^
    //              set bit

    AddressDecomposition result = cache.decompose(0x10);

    assert(result.offset == 0);
    assert(result.set_index == 1);
    assert(result.tag == 0);
}

void test_set_rollover()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // 0x20 is the third cache line.
    // With two sets, it maps back to set 0.
    AddressDecomposition result = cache.decompose(0x20);

    assert(result.offset == 0);
    assert(result.set_index == 0);
    assert(result.tag == 1);
}

void test_tag_decomposition()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // 0x20 = third cache line.
    // Line 0 -> set 0, tag 0
    // Line 1 -> set 1, tag 0
    // Line 2 -> set 0, tag 1

    AddressDecomposition result = cache.decompose(0x20);

    assert(result.offset == 0);
    assert(result.set_index == 0);
    assert(result.tag == 1);
}

void test_offset_set_and_tag()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // Address = 0x35
    //
    // Binary:
    // 0x35 = 0011 0101
    //
    // offset = lower 4 bits = 0101 = 5
    // set    = next bit    = 1
    // tag    = remaining   = 1

    AddressDecomposition result = cache.decompose(0x35);

    assert(result.offset == 5);
    assert(result.set_index == 1);
    assert(result.tag == 1);
}

/*
void test_cache_geometry()
{
    SystemConfig config{};
    Cache cache{config, 0};

    assert(cache.m_sets.size() == config.cache_set_count);

    for (const auto& set : cache.m_sets)
    {
        assert(set.size() == config.cache_associativity);

        for (const auto& line : set)
        {
            assert(line.data.size() == config.cache_line_size);
        }
    }
}

void test_cache_lines_initialised()
{
    SystemConfig config{};
    Cache cache{config, 0};

    for (const auto& set : cache.m_sets)
    {
        for (const auto& line : set)
        {
            for (const auto byte : line.data)
            {
                assert(byte == 0);
            }
        }
    }
}

void test_custom_cache_geometry()
{
    SystemConfig config{};

    config.cache_line_size = 32;
    config.cache_set_count = 4;
    config.cache_associativity = 3;

    Cache cache{config, 0};

    assert(cache.m_sets.size() == 4);

    for (const auto& set : cache.m_sets)
    {
        assert(set.size() == 3);

        for (const auto& line : set)
        {
            assert(line.data.size() == 32);
        }
    }
}
*/

void test_empty_cache_miss()
{
    SystemConfig config{};
    Cache cache{config, 0};

    assert(!cache.contains(0x00));
    assert(!cache.contains(0x10));
    assert(!cache.contains(0x20));
}

void test_install_line_creates_hit()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.contains(0x20));
}

void test_install_line_covers_entire_line()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.contains(0x20));
    assert(cache.contains(0x21));
    assert(cache.contains(0x2F));
}

void test_install_line_different_line_is_miss()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.contains(0x20));
    assert(!cache.contains(0x30));
}

void test_install_multiple_lines_same_set()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x00, line, MESIState::Exclusive);
    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.contains(0x00));
    assert(cache.contains(0x20));
}

void test_install_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Exclusive);
}

void test_find_line_empty_cache()
{
    SystemConfig config{};
    Cache cache{config, 0};

    assert(cache.find_line(0x20) == nullptr);
}

void test_find_line_returns_installed_line()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
}

void test_find_line_returns_correct_data()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0] = 0xAA;
    line.data[5] = 0x42;
    line.data[15] = 0xFF;

    cache.install_line(0x20, line, MESIState::Exclusive);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->data[0] == 0xAA);
    assert(result->data[5] == 0x42);
    assert(result->data[15] == 0xFF);
}

void test_find_line_different_line()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.find_line(0x30) == nullptr);
}

void test_find_line_same_cache_line()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    const CacheLine* first = cache.find_line(0x20);
    const CacheLine* second = cache.find_line(0x25);
    const CacheLine* third = cache.find_line(0x2F);

    assert(first != nullptr);
    assert(second != nullptr);
    assert(third != nullptr);

    assert(first == second);
    assert(second == third);
}

void test_read_installed_byte()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.data[0] = 0xAA;

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.read(0x20) == 0xAA);
}

void test_read_different_offsets()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0]  = 0x11;
    line.data[5]  = 0x55;
    line.data[15] = 0xFF;

    cache.install_line(0x20, line, MESIState::Exclusive);

    assert(cache.read(0x20) == 0x11);
    assert(cache.read(0x25) == 0x55);
    assert(cache.read(0x2F) == 0xFF);
}

void test_read_multiple_lines()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line1{};
    line1.data.resize(config.cache_line_size);
    line1.data[0] = 0xAA;

    CacheLine line2{};
    line2.data.resize(config.cache_line_size);
    line2.data[0] = 0xBB;

    cache.install_line(0x20, line1, MESIState::Exclusive);
    cache.install_line(0x30, line2, MESIState::Exclusive);

    assert(cache.read(0x20) == 0xAA);
    assert(cache.read(0x30) == 0xBB);
}

void test_write_and_read()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    cache.write(0x20, 0xAB);

    assert(cache.read(0x20) == 0xAB);
}

void test_write_different_offsets()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    cache.write(0x20, 0x11);
    cache.write(0x25, 0x55);
    cache.write(0x2F, 0xFF);

    assert(cache.read(0x20) == 0x11);
    assert(cache.read(0x25) == 0x55);
    assert(cache.read(0x2F) == 0xFF);
}

void test_write_preserves_other_bytes()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0] = 0xAA;
    line.data[1] = 0xBB;

    cache.install_line(0x20, line, MESIState::Exclusive);

    cache.write(0x20, 0x11);

    assert(cache.read(0x20) == 0x11);
    assert(cache.read(0x21) == 0xBB);
}

void test_write_multiple_lines()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line1{};
    line1.data.resize(config.cache_line_size);

    CacheLine line2{};
    line2.data.resize(config.cache_line_size);

    cache.install_line(0x20, line1, MESIState::Exclusive);
    cache.install_line(0x30, line2, MESIState::Exclusive);

    cache.write(0x20, 0xAA);
    cache.write(0x30, 0xBB);

    assert(cache.read(0x20) == 0xAA);
    assert(cache.read(0x30) == 0xBB);
}

void test_read_hit_preserves_shared_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Shared);

    cache.read(0x20);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Shared);
}

void test_read_hit_preserves_exclusive_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    cache.read(0x20);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Exclusive);
}

void test_read_hit_preserves_modified_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Modified);

    cache.read(0x20);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Modified);
}

void test_write_shared_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Shared);

    cache.write(0x20, 0xAB);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Shared);
    assert(cache.read(0x20) == 0xAB);
}

void test_write_exclusive_transitions_to_modified()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Exclusive);

    cache.write(0x20, 0xAB);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Modified);
    assert(cache.read(0x20) == 0xAB);
}

void test_write_modified_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line, MESIState::Modified);

    cache.write(0x20, 0xAB);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->state == MESIState::Modified);
    assert(cache.read(0x20) == 0xAB);
}

void test_snoop_busrd_shared_state()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.data[0] = 42;

    cache.install_line(0x00, line, MESIState::Shared);

    cache.snoop(Transaction::BusRd, 0x00);

    const CacheLine* result{cache.find_line(0x00)};

    assert(result != nullptr);
    assert(result->state == MESIState::Shared);
}

void test_snoop_busrd_exclusive_to_shared()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Exclusive);

    cache.snoop(Transaction::BusRd, 0x00);

    const CacheLine* result{cache.find_line(0x00)};

    assert(result != nullptr);
    assert(result->state == MESIState::Shared);
}

void test_snoop_busrd_modified_to_shared()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.data[0] = 42;

    cache.install_line(0x00, line, MESIState::Modified);

    cache.snoop(Transaction::BusRd, 0x00);

    const CacheLine* result{cache.find_line(0x00)};

    assert(result != nullptr);
    assert(result->state == MESIState::Shared);

    // Snoop should not modify the cached data.
    assert(result->data[0] == 42);
}

void test_snoop_busrdx_shared_to_invalid()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Shared);

    cache.snoop(Transaction::BusRdX, 0x00);

    assert(!cache.contains(0x00));
}

void test_snoop_busrdx_exclusive_to_invalid()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Exclusive);

    cache.snoop(Transaction::BusRdX, 0x00);

    assert(!cache.contains(0x00));
}

void test_snoop_busrdx_modified_to_invalid()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.data[0] = 42;

    cache.install_line(0x00, line, MESIState::Modified);

    cache.snoop(Transaction::BusRdX, 0x00);

    assert(!cache.contains(0x00));
}

void test_snoop_busupgr_shared_to_invalid()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Shared);

    cache.snoop(Transaction::BusUpgr, 0x00);

    assert(!cache.contains(0x00));
}

void test_snoop_busupgr_exclusive_to_invalid()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Exclusive);

    cache.snoop(Transaction::BusUpgr, 0x00);

    assert(!cache.contains(0x00));
}

void test_snoop_busupgr_modified_unchanged()
{
    SystemConfig config{};
    Cache cache{config, 0};

    CacheLine line{};

    cache.install_line(0x00, line, MESIState::Modified);

    cache.snoop(Transaction::BusUpgr, 0x00);

    const CacheLine* result{cache.find_line(0x00)};

    assert(result != nullptr);
    assert(result->state == MESIState::Modified);
}

void test_snoop_absent_line()
{
    SystemConfig config{};
    Cache cache{config, 0};

    // There is no line at this address.
    assert(!cache.contains(0x00));

    // Snoop should simply do nothing rather than dereferencing nullptr.
    cache.snoop(Transaction::BusRd, 0x00);
    cache.snoop(Transaction::BusRdX, 0x00);
    cache.snoop(Transaction::BusUpgr, 0x00);

    assert(!cache.contains(0x00));
}

int main()
{
    // test address decomposition
    test_address_zero();
    test_offset_decomposition();
    test_offset_rollover();
    test_set_decomposition();
    test_set_rollover();
    test_tag_decomposition();
    test_offset_set_and_tag();

    // test cache geometry
    /*
    test_cache_geometry();
    test_cache_lines_initialised();
    test_custom_cache_geometry();
    */

    // test cache lookup
    test_empty_cache_miss();

    // test install_line()
    test_install_line_creates_hit();
    test_install_line_covers_entire_line();
    test_install_line_different_line_is_miss();
    test_install_multiple_lines_same_set();
    test_install_state();

    // test find_line()
    test_find_line_empty_cache();
    test_find_line_returns_installed_line();
    test_find_line_returns_correct_data();
    test_find_line_different_line();
    test_find_line_same_cache_line();

    // test read()
    test_read_installed_byte();
    test_read_different_offsets();
    test_read_multiple_lines();

    // test write()
    test_write_and_read();
    test_write_different_offsets();
    test_write_preserves_other_bytes();
    test_write_multiple_lines();

    // test MESI read-hit transitions
    test_read_hit_preserves_shared_state();
    test_read_hit_preserves_exclusive_state();
    test_read_hit_preserves_modified_state();

    // test MESI write-hit transitions
    test_write_shared_state();
    test_write_exclusive_transitions_to_modified();
    test_write_modified_state();

    // test snooping
    test_snoop_busrd_shared_state();
    test_snoop_busrd_exclusive_to_shared();
    test_snoop_busrd_modified_to_shared();

    test_snoop_busrdx_shared_to_invalid();
    test_snoop_busrdx_exclusive_to_invalid();
    test_snoop_busrdx_modified_to_invalid();

    test_snoop_busupgr_shared_to_invalid();
    test_snoop_busupgr_exclusive_to_invalid();
    test_snoop_busupgr_modified_unchanged();

    test_snoop_absent_line();

    std::cout << "All cache tests passed!\n";

    return 0;
}