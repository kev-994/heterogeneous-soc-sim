#include "cache.hpp"

#include <cstdint>
#include <cassert>
#include <array>

AddressDecomp decompose(std::uint32_t address)
{
    AddressDecomp decomp{};

    decomp.offset    =  address & 0xF;       // lower 4 bits
    decomp.set_index = (address >> 4) & 0x1; // next bit
    decomp.tag       =  address >> 5;        // upper 27 bits

    return decomp;
}

void Cache::write_memory(std::uint32_t address, std::uint8_t data)
{
    assert(address < m_memory.size());
    m_memory[address] = data;
}

void Cache::install_cache_line(CacheLine& line, std::uint32_t line_base, std::uint32_t tag)
{
    // copy the data that's in memory
    for (std::size_t i{}; i < line.data.size(); ++i)
    {
        line.data[i] = m_memory[line_base+i];
    }

    // cache line fill
    line.valid = true;
    line.tag   = tag;
}


Cache::CacheResult Cache::read(std::uint32_t address)
{
    const auto decomposedAddress{decompose(address)};
    std::uint32_t line_base = address & ~0xF;

    const auto set = decomposedAddress.set_index;
    const auto tag = decomposedAddress.tag;       // requested tag

    for (const auto& way : m_cache[set]) // way is a CacheLine
    {
        if (way.valid && way.tag == tag)          // way.tag is the CacheLine tag (stored tag)
            return CacheResult::Hit;
    }

    // cache line fill
    for (auto& way : m_cache[set])
    {
        if (!way.valid)
        {
            install_cache_line(way, line_base, tag);
            return CacheResult::Miss;
        }
    }

    // replace way 0 if both ways are valid
    install_cache_line(m_cache[set][0], line_base, tag);

    return CacheResult::Miss;
};