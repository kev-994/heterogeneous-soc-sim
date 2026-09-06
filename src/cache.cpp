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
    line.dirty = false;
}

// way 0 evection
void Cache::evict(CacheLine& line, std::uint32_t set)
{
    if (line.dirty)
    {
        const std::uint32_t old_line_base = (line.tag << 5) | (set << 4);

        // write old cache line to memory if dirty
        for (std::size_t i{}; i < line.data.size(); ++i)
        {
            m_memory[old_line_base+i] = line.data[i]; 
        }
    }
}


Cache::ReadResult Cache::read(std::uint32_t address)
{
    const auto decomposedAddress{decompose(address)};
    const std::uint32_t line_base = address & ~0xF;
    ReadResult read_result{};

    const auto set = decomposedAddress.set_index;
    const auto tag = decomposedAddress.tag;       // requested tag

    for (const auto& way : m_cache[set]) // way is a CacheLine
    {
        if (way.valid && way.tag == tag)          // way.tag is the CacheLine tag (stored tag)
        {
            read_result.result = CacheResult::Hit;
            read_result.data   = way.data[decomposedAddress.offset];
            return read_result;
        }
    }

    // cache line fill
    for (auto& way : m_cache[set])
    {
        if (!way.valid)
        {
            install_cache_line(way, line_base, tag);
            read_result.result = CacheResult::Miss;
            read_result.data   = way.data[decomposedAddress.offset];
            return read_result;
        }
    }

    // replace way 0 if both ways are valid
    auto& way0{m_cache[set][0]};
    evict(way0, set);
    install_cache_line(way0, line_base, tag);
    read_result.result = CacheResult::Miss;
    read_result.data   = way0.data[decomposedAddress.offset];
    return read_result;
};

// write-back
void Cache::write(std::uint32_t address, std::uint8_t data)
{
    const auto decomposedAddress{decompose(address)};
    const std::uint32_t line_base = address & ~0xF;

    const auto set = decomposedAddress.set_index;
    const auto tag = decomposedAddress.tag;       // requested tag

    // write hit
    for (auto& way : m_cache[set])
    {
        // write hit
        if (way.valid && way.tag == tag)
        {
            way.data[decomposedAddress.offset] = data;
            way.dirty = true;
            return;
        }
    }

    // empty way write-miss
    for (auto& way : m_cache[set])
    {
        if (!way.valid)
        {
            install_cache_line(way, line_base, tag);
            way.data[decomposedAddress.offset] = data;
            way.dirty = true;
            return;
        }
    }

    // way 0 eviction write-miss
    auto& way0{m_cache[set][0]};
    evict(way0, set);
    install_cache_line(way0, line_base, tag);
    way0.data[decomposedAddress.offset] = data;
    way0.dirty = true;
}

std::uint8_t getByte(const Cache& cache, std::uint32_t address)
{
    return cache.m_memory[address];
}