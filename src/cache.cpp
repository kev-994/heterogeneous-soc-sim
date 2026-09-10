#include "cache.hpp"

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

void Cache::install_line(Address address, const CacheLine& line, MESIState state)
{
    const auto decomposed_address{decompose(address)};

    const auto tag = decomposed_address.tag;
    const auto set_index = decomposed_address.set_index;

    for (auto& line_ : m_sets[set_index])
    {
        if (line_.state == MESIState::Invalid)
        {
            line_.data = line.data;
            line_.tag = tag;
            line_.state = state;
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
        if (line.state != MESIState::Invalid && line.tag == tag)
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
        if (line.state != MESIState::Invalid && line.tag == tag)
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

    handle_read_hit(*line);

    const auto decomposed{decompose(address)};

    return line->data[decomposed.offset];
}

void Cache::write(Address address, std::uint8_t data)
{
    CacheLine* line{find_line(address)};
    assert(line != nullptr);
    
    handle_write_hit(*line);

    const auto decomposed{decompose(address)};

    line->data[decomposed.offset] = data;
}


void Cache::handle_read_hit(const CacheLine& line) const
{
    assert(line.state != MESIState::Invalid);

    using enum MESIState;

    switch (line.state)
    {
        case Shared:
        case Exclusive:
        case Modified:
            return;

        case Invalid:
            assert(false);
            return;
    }
}

void Cache::handle_write_hit(CacheLine& line)
{
    assert(line.state != MESIState::Invalid);
    
    using enum MESIState;
    switch (line.state)
    {
        case Invalid:
        {    
            assert(false);
            return;
        }    
        case Shared:
        {
            return;
        }
        case Exclusive: 
        {
            line.state = Modified;
            return;
        }
        case Modified:
        {
            return;
        }
    }
}

SnoopResult Cache::snoop(Transaction transaction_type, Address address) // snoop target 
{
    SnoopResult result{};
    
    CacheLine* line{find_line(address)};

    if (line == nullptr)
    {
        return result;
    }

    auto& state{line->state};
    
    // find_line() never returns a line that is invalid, any such case is defensive coding
    using enum Transaction;
    switch (transaction_type)
    {
        case BusRd:
        {
            switch (state)
            {
                case MESIState::Invalid:
                {
                    return result;
                }
                case MESIState::Shared: [[fallthrough]];
                case MESIState::Exclusive: 
                {
                    result.copy_exists = true;
                    state = MESIState::Shared;
                    return result;
                }
                case MESIState::Modified:
                {
                    result.copy_exists = true;
                    result.supplies_data = true;
                    result.line_data.data = line->data;
                    state = MESIState::Shared;
                    return result;
                }   
            }
        }
        
        case BusRdX:
        {
            state = MESIState::Invalid;
            return result;
        }

        case BusUpgr:
        {
            switch (state)
            {
                case MESIState::Modified:
                {
                    assert(false); // this shouldn't happen
                    return result;
                }
                case MESIState::Shared: [[fallthrough]];
                case MESIState::Exclusive: 
                {
                    result.copy_exists = true;
                    state = MESIState::Invalid;
                    return result;
                }
                case MESIState::Invalid: 
                {
                    state = MESIState::Invalid;
                    return result;
                }   
            }   
        }
        
    }
}