#include "cache.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

void test_address_zero()
{
    SystemConfig config{};
    Cache cache{config};

    AddressDecomposition result = cache.decompose(0);

    assert(result.offset == 0);
    assert(result.set_index == 0);
    assert(result.tag == 0);
}

void test_offset_decomposition()
{
    SystemConfig config{};
    Cache cache{config};

    // 16-byte cache line -> 4 offset bits.
    AddressDecomposition result = cache.decompose(0x0F);

    assert(result.offset == 0x0F);
    assert(result.set_index == 0);
    assert(result.tag == 0);
}

void test_offset_rollover()
{
    SystemConfig config{};
    Cache cache{config};

    // Address 0x10 is the start of the next cache line.
    AddressDecomposition result = cache.decompose(0x10);

    assert(result.offset == 0);
    assert(result.set_index == 1);
    assert(result.tag == 0);
}

void test_set_decomposition()
{
    SystemConfig config{};
    Cache cache{config};

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
    Cache cache{config};

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
    Cache cache{config};

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
    Cache cache{config};

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
    Cache cache{config};

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
    Cache cache{config};

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

    Cache cache{config};

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
    Cache cache{config};

    assert(!cache.contains(0x00));
    assert(!cache.contains(0x10));
    assert(!cache.contains(0x20));
}

void test_install_line_creates_hit()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    assert(cache.contains(0x20));
}

void test_install_line_covers_entire_line()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    assert(cache.contains(0x20));
    assert(cache.contains(0x21));
    assert(cache.contains(0x2F));
}

void test_install_line_different_line_is_miss()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    assert(cache.contains(0x20));
    assert(!cache.contains(0x30));
}

void test_install_multiple_lines_same_set()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x00, line);
    cache.install_line(0x20, line);

    assert(cache.contains(0x00));
    assert(cache.contains(0x20));
}

void test_find_line_empty_cache()
{
    SystemConfig config{};
    Cache cache{config};

    assert(cache.find_line(0x20) == nullptr);
}

void test_find_line_returns_installed_line()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
}

void test_find_line_returns_correct_data()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0] = 0xAA;
    line.data[5] = 0x42;
    line.data[15] = 0xFF;

    cache.install_line(0x20, line);

    const CacheLine* result = cache.find_line(0x20);

    assert(result != nullptr);
    assert(result->data[0] == 0xAA);
    assert(result->data[5] == 0x42);
    assert(result->data[15] == 0xFF);
}

void test_find_line_different_line()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    assert(cache.find_line(0x30) == nullptr);
}

void test_find_line_same_cache_line()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

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
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);
    line.data[0] = 0xAA;

    cache.install_line(0x20, line);

    assert(cache.read(0x20) == 0xAA);
}

void test_read_different_offsets()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0]  = 0x11;
    line.data[5]  = 0x55;
    line.data[15] = 0xFF;

    cache.install_line(0x20, line);

    assert(cache.read(0x20) == 0x11);
    assert(cache.read(0x25) == 0x55);
    assert(cache.read(0x2F) == 0xFF);
}

void test_read_multiple_lines()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line1{};
    line1.data.resize(config.cache_line_size);
    line1.data[0] = 0xAA;

    CacheLine line2{};
    line2.data.resize(config.cache_line_size);
    line2.data[0] = 0xBB;

    cache.install_line(0x20, line1);
    cache.install_line(0x30, line2);

    assert(cache.read(0x20) == 0xAA);
    assert(cache.read(0x30) == 0xBB);
}

void test_write_and_read()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

    cache.write(0x20, 0xAB);

    assert(cache.read(0x20) == 0xAB);
}

void test_write_different_offsets()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    cache.install_line(0x20, line);

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
    Cache cache{config};

    CacheLine line{};
    line.data.resize(config.cache_line_size);

    line.data[0] = 0xAA;
    line.data[1] = 0xBB;

    cache.install_line(0x20, line);

    cache.write(0x20, 0x11);

    assert(cache.read(0x20) == 0x11);
    assert(cache.read(0x21) == 0xBB);
}

void test_write_multiple_lines()
{
    SystemConfig config{};
    Cache cache{config};

    CacheLine line1{};
    line1.data.resize(config.cache_line_size);

    CacheLine line2{};
    line2.data.resize(config.cache_line_size);

    cache.install_line(0x20, line1);
    cache.install_line(0x30, line2);

    cache.write(0x20, 0xAA);
    cache.write(0x30, 0xBB);

    assert(cache.read(0x20) == 0xAA);
    assert(cache.read(0x30) == 0xBB);
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

    // test read()
    test_write_and_read();
    test_write_different_offsets();
    test_write_preserves_other_bytes();
    test_write_multiple_lines();


    std::cout << "All cache tests passed!\n";

    return 0;
}