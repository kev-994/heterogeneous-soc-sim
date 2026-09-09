#include "cache.hpp"

#include "utils.hpp"

#include <cassert>
#include <cstdint>

AddressDecomposition Cache::decompose(Address address) const
{
    AddressDecomposition decomposed_address{};

    decomposed_address.offset = 
        address & ((1u << m_offset_bits) - 1);
    
    decomposed_address.set_index = 
        (address >> m_offset_bits) & ((1u << m_set_bits) - 1);
    
    decomposed_address.tag = 
        address >> (m_offset_bits + m_set_bits);

    return decomposed_address;
}

bool Cache::contains(Address address) const
{
    return find_line(address) != nullptr;
}

void Cache::install_line(Address address, const CacheLine& line)
{
    const auto decomposed_address{decompose(address)};

    const auto tag = decomposed_address.tag;
    const auto set_index = decomposed_address.set_index;

    for (auto& line_ : m_sets[set_index])
    {
        if (!line_.valid)
        {
            line_.data = line.data;
            line_.tag = tag;
            line_.valid = true;
            return;
        }
    }

    assert(false); // no free way
}

CacheLine* Cache::find_line(Address address)
{
    const auto decomposed_address{decompose(address)};

    const auto tag = decomposed_address.tag;
    const auto set_index = decomposed_address.set_index;

    for (auto& line : m_sets[set_index])
    {
        if (line.valid && line.tag == tag)
        {
            return &line;
        }
    }

    return nullptr;
}

const CacheLine* Cache::find_line(Address address) const
{
    const auto decomposed_address{decompose(address)};

    const auto tag = decomposed_address.tag;
    const auto set_index = decomposed_address.set_index;

    for (const auto& line : m_sets[set_index])
    {
        if (line.valid && line.tag == tag)
        {
            return &line;
        }
    }

    return nullptr;
}

std::uint8_t Cache::read(Address address) const
{
    const CacheLine* line{find_line(address)};
    assert(line != nullptr);

    const auto decomposed{decompose(address)};

    return line->data[decomposed.offset];
}

void Cache::write(Address address, std::uint8_t data)
{
    CacheLine* line{find_line(address)};
    assert(line != nullptr);

    const auto decomposed{decompose(address)};

    line->data[decomposed.offset] = data;
}