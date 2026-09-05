#pragma once

#include <cstdint>
#include <array>

struct AddressDecomp
{
    std::uint32_t tag{};
    std::uint8_t  set_index{};
    std::uint8_t  offset{};
};

struct CacheLine
{
    bool                          valid{};
    std::uint32_t                 tag{};
    std::array<std::uint8_t, 16>  data{}; // 16 1-byte integers
};

class Cache
{
public:
    enum class CacheResult
    {
        Hit,
        Miss
    };

    CacheResult read(std::uint32_t address);
    void write_memory(std::uint32_t address, std::uint8_t data);
    void install_cache_line(CacheLine& line, std::uint32_t line_base, std::uint32_t tag);

private:
    // 2 sets × 2 ways of CacheLine
    std::array<std::array<CacheLine, 2>, 2> m_cache{};
    std::array<std::uint8_t, 256> m_memory{};
};

AddressDecomp decompose(std::uint32_t address);