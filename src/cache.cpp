#include "cache.hpp"

#include <cstdint>

AddressDecomp decompose(std::uint32_t address)
{
    AddressDecomp decomp{};

    decomp.offset    =  address & 0xF;       // lower 4 bits
    decomp.set_index = (address >> 4) & 0x1; // next bit
    decomp.tag       =  address >> 5;        // upper 27 bits

    return decomp;
}


Cache::CacheResult Cache::read(std::uint32_t address)
{
    AddressDecomp decomposedAddress{decompose(address)};

    const auto set = decomposedAddress.set_index;
    const auto tag = decomposedAddress.tag;       // requested tag

    for (const auto& way : m_cache[set])
    {
        if (way.valid && way.tag == tag)          // way.tag is the CacheLine tag (stored tag)
            return CacheResult::Hit;
    }

    return CacheResult::Miss;
};