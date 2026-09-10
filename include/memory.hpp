#pragma once

#include "types.hpp"
#include "config.hpp"

#include <cstdint>
#include <vector>
#include <cassert>

struct LineData // = the bytes belonging to a cache line being transferred between components.
{
    std::vector<std::uint8_t> data{};
};

class Memory
{
public:
    Memory(const SystemConfig& config)
        : m_config{config}, m_memory(config.memory_capacity, 0)
    {}

    std::uint8_t read_byte(Address address) const;
    void write_byte(Address address, std::uint8_t data);

    LineData read_line(Address line_base) const;
    void write_line(Address line_base, LineData line_data);

private: 
    SystemConfig m_config{};
    std::vector<std::uint8_t> m_memory{};

};