#pragma once

#include <array>
#include <cstdint>

class Memory
{
public:
    void write(std::uint32_t address, std::uint8_t data);
    std::uint8_t get_byte(std::uint32_t address) const;
private:
    std::array<std::uint8_t, 256> m_memory{};
};