#include "memory.hpp"

#include <cassert>
#include <cstdint>

std::uint8_t Memory::read_byte(Address address) const
{
    assert(address < m_config.memory_capacity);
    return m_memory[address];
}

void Memory::write_byte(Address address, std::uint8_t data)
{
    assert(address < m_config.memory_capacity);
    m_memory[address] = data;
}

Memory::LineData Memory::read_line(Address line_base) const
{
    assert(line_base % m_config.cache_line_size == 0); // cache line alignment
    assert(line_base + m_config.cache_line_size <= m_config.memory_capacity); // validity of line to be accessed
    
    LineData line_data{};
    line_data.data.resize(m_config.cache_line_size); // so ith element can be accessed

    for (std::uint32_t i{}; i < m_config.cache_line_size; ++i)
    {
        line_data.data[i] = m_memory[line_base+i];
    }

    return line_data;
}

void Memory::write_line(Address line_base, LineData line_data)
{
    assert(line_data.data.size() == m_config.cache_line_size); // line data size must match configurable
    assert(line_base % m_config.cache_line_size == 0);
    assert(line_base + m_config.cache_line_size <= m_config.memory_capacity);

    for (std::uint32_t i{}; i < m_config.cache_line_size; ++i)
    {
        m_memory[line_base+i] = line_data.data[i];
    }    
}