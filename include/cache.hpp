#pragma once

#include "types.hpp"
#include "config.hpp"
#include "utils.hpp"

#include <cstdint>
#include <cassert>
#include <vector>

struct AddressDecomposition
{
    Address offset;
    Address set_index;
    Address tag;
};

struct CacheLine
{
    std::uint32_t tag{};
    std::vector<std::uint8_t> data{};
    bool valid{};
};

class Cache
{
public:
    Cache(const SystemConfig& config)
        : m_config{config}, m_sets(config.cache_set_count)
    {
        assert(is_power_of_two(config.cache_line_size)); // cache geometry validation
        assert(is_power_of_two(config.cache_set_count));
        
        m_offset_bits = clog2(config.cache_line_size);
        m_set_bits = clog2(config.cache_set_count);

        // cache geometry
        for (auto& set : m_sets)
        {
            set.resize(config.cache_associativity);

            for (auto& way : set)
            {
                way.data.resize(config.cache_line_size);
            }
        }
    }

    AddressDecomposition decompose(Address address) const;

    bool contains(Address address) const;
    void install_line(Address address, const CacheLine& line);
    CacheLine* find_line(Address address); 
    const CacheLine* find_line(Address address) const; // used by find_line() and contains()
    std::uint8_t read(Address address) const;
    void write(Address address, std::uint8_t data);

private:
    SystemConfig m_config{};
    std::vector<std::vector<CacheLine>> m_sets{};

    std::uint32_t m_offset_bits{};
    std::uint32_t m_set_bits{};

};