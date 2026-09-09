#include "memory.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

void test_initial_memory_is_zero()
{
    SystemConfig config{};
    Memory memory{config};

    for (Address address = 0; address < config.memory_capacity; ++address)
    {
        assert(memory.read_byte(address) == 0);
    }
}

void test_read_write_byte()
{
    SystemConfig config{};
    Memory memory{config};

    memory.write_byte(10, 0xAB);

    assert(memory.read_byte(10) == 0xAB);
}

void test_multiple_byte_writes()
{
    SystemConfig config{};
    Memory memory{config};

    memory.write_byte(0, 0x12);
    memory.write_byte(1, 0x34);
    memory.write_byte(2, 0x56);
    memory.write_byte(3, 0x78);

    assert(memory.read_byte(0) == 0x12);
    assert(memory.read_byte(1) == 0x34);
    assert(memory.read_byte(2) == 0x56);
    assert(memory.read_byte(3) == 0x78);
}

void test_read_line()
{
    SystemConfig config{};
    Memory memory{config};

    // Fill one 16-byte line with known data.
    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        memory.write_byte(i, static_cast<std::uint8_t>(i + 1));
    }

    Memory::LineData line = memory.read_line(0);

    assert(line.data.size() == config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        assert(line.data[i] == static_cast<std::uint8_t>(i + 1));
    }
}

void test_write_line()
{
    SystemConfig config{};
    Memory memory{config};

    Memory::LineData line{};
    line.data.resize(config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        line.data[i] = static_cast<std::uint8_t>(0xA0 + i);
    }

    assert(line.data.size() == config.cache_line_size);

    memory.write_line(0, line);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        assert(memory.read_byte(i) ==
               static_cast<std::uint8_t>(0xA0 + i));
    }
}

void test_read_write_line()
{
    SystemConfig config{};
    Memory memory{config};

    Memory::LineData write_data{};
    write_data.data.resize(config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        write_data.data[i] = static_cast<std::uint8_t>(i * 3);
    }

    memory.write_line(16, write_data);

    Memory::LineData read_data = memory.read_line(16);

    assert(read_data.data.size() == config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        assert(read_data.data[i] == write_data.data[i]);
    }
}

void test_last_memory_line()
{
    SystemConfig config{};
    Memory memory{config};

    Address last_line =
        config.memory_capacity - config.cache_line_size;

    Memory::LineData line{};
    line.data.resize(config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        line.data[i] = static_cast<std::uint8_t>(i + 1);
    }

    memory.write_line(last_line, line);

    Memory::LineData read_data = memory.read_line(last_line);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        assert(read_data.data[i] == line.data[i]);
    }
}

void test_aligned_line()
{
    SystemConfig config{};
    Memory memory{config};

    Address line_base = config.cache_line_size * 1;

    Memory::LineData line{};
    line.data.resize(config.cache_line_size);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        line.data[i] = static_cast<std::uint8_t>(i);
    }

    memory.write_line(line_base, line);

    Memory::LineData read_data = memory.read_line(line_base);

    for (std::uint32_t i = 0; i < config.cache_line_size; ++i)
    {
        assert(read_data.data[i] == line.data[i]);
    }
}

void test_independent_lines()
{
    SystemConfig config{};
    Memory memory{config};

    Memory::LineData line_a{};
    line_a.data.resize(config.cache_line_size);

    Memory::LineData line_b{};
    line_b.data.resize(config.cache_line_size);

    line_a.data[0] = 0xAA;
    line_b.data[0] = 0xBB;

    memory.write_line(0, line_a);
    memory.write_line(config.cache_line_size, line_b);

    assert(memory.read_line(0).data[0] == 0xAA);
    assert(memory.read_line(config.cache_line_size).data[0] == 0xBB);
}

int main()
{
    test_initial_memory_is_zero();
    test_read_write_byte();
    test_multiple_byte_writes();
    test_read_line();
    test_write_line();
    test_read_write_line();
    test_last_memory_line();
    test_aligned_line();
    test_independent_lines();

    std::cout << "All memory tests passed!\n";

    return 0;
}