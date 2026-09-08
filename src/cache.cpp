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


void Cache::install_cache_line(CacheLine& line, std::uint32_t line_base, std::uint32_t tag)
{
    // copy the data that's in memory
    for (std::size_t i{}; i < line.data.size(); ++i)
    {
        line.data[i] = m_memory.get_byte(line_base+i);
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
            m_memory.write(old_line_base+i, line.data[i]); 
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

    for (auto& way : m_cache[set]) // way is a CacheLine
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
            way.state = MESIState::Exclusive;
            return read_result;
        }
    }

    // replace way 0 if both ways are valid
    auto& way0{m_cache[set][0]};
    evict(way0, set);
    install_cache_line(way0, line_base, tag);
    read_result.result = CacheResult::Miss;
    read_result.data   = way0.data[decomposedAddress.offset];
    way0.state = MESIState::Exclusive;
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
            way.state = MESIState::Modified;
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
            way.state = MESIState::Modified;
            return;
        }
    }

    // way 0 eviction write-miss
    auto& way0{m_cache[set][0]};
    evict(way0, set);
    install_cache_line(way0, line_base, tag);
    way0.data[decomposedAddress.offset] = data;
    way0.dirty = true;
    way0.state = MESIState::Modified;
}

MESIState Cache::get_state(std::uint32_t address) const
{
    // find address in this cache
    const auto decomposed_address{decompose(address)};

    const auto set = decomposed_address.set_index;
    const auto tag = decomposed_address.tag;
    
    for (const auto& way : m_cache[set])
    {
        if (way.valid && way.tag == tag)
        {
            return way.state;
        }
    }

    return MESIState::Invalid;
}

// handles coherence and memory consistency
Cache::SnoopResult Cache::snoop(BusRequest request, std::uint32_t address)
{
    // find address in this cache
    const auto decomposed_address{decompose(address)};

    const auto set = decomposed_address.set_index;
    const auto tag = decomposed_address.tag;

    SnoopResult snoop_result{};

    for (auto& way : m_cache[set])
    {
        // Do I have the requested cache line?
        if (way.valid && way.tag == tag)
        {
            // Another cache wants to read it.
            if (request == BusRequest::BusRd)
            {
                // S -> S
                {
                    if (way.state == MESIState::Shared)
                    {
                        // not entirely necessary
                        way.state = MESIState::Shared;
                        snoop_result.hit = true;
                        snoop_result.data = way.data;
                    }
                }
                
                // E -> S
                if (way.state == MESIState::Exclusive)
                {
                    way.state = MESIState::Shared;
                    snoop_result.hit = true;
                    snoop_result.data = way.data;
                }


                // M -> S
                else if (way.state == MESIState::Modified)
                {
                    way.state = MESIState::Shared;
                    way.dirty = false;
                    snoop_result.hit = true;
                    snoop_result.data = way.data;

                    // update memory with new data
                    const std::uint32_t line_base = (way.tag << 5) | (set << 4);

                    for (std::size_t i{}; i < way.data.size(); ++i)
                    {
                        m_memory.write(line_base+i, way.data[i]); 
                    }
                }
            }

            return snoop_result;
        }
    }

    return snoop_result;
}

void install_received_line(CacheLine& line, const std::array<std::uint8_t, 16>& data, std::uint32_t tag)
{
    line.data  = data;
    line.valid = true;
    line.tag   = tag;
    line.dirty = false;
    line.state = MESIState::Shared;

}

void Cache::receive_line(std::uint32_t address, const std::array<uint8_t, 16>& data)
{
    // find address in this cache
    const auto decomposed_address{decompose(address)};

    const auto set = decomposed_address.set_index;
    const auto tag = decomposed_address.tag;

    for (auto& way : m_cache[set])
    {
        if (way.valid && way.tag == tag)
        {
            install_received_line(way, data, tag);
            return;
        }
    }

    for (auto& way : m_cache[set])
    {
        if (!way.valid)
        {
            install_received_line(way, data, tag);
            return;
        }
    }

    // way 0 replacement
    auto& way0{m_cache[set][0]};
    evict(way0, set);
    install_received_line(way0, data, tag);
}