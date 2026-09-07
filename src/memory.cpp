#include "memory.hpp"

#include <cassert>

void Memory::write(std::uint32_t address, std::uint8_t data)
{
    assert(address < m_memory.size());
    m_memory[address] = data;
}

std::uint8_t Memory::get_byte(std::uint32_t address) const
{
    assert(address < m_memory.size());
    return m_memory[address];
}