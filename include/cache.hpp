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

enum class MESIState
{
    Invalid,
    Shared,
    Exclusive,
    Modified
};

struct CacheLine
{
    std::uint32_t tag{};
    std::vector<std::uint8_t> data{};
    MESIState state{MESIState::Invalid};
};

class Cache
{
public:
    Cache(const SystemConfig& config, AgentId agent_id)
        : m_config{config}, m_sets(config.cache_set_count), m_agent_id{agent_id}
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
    void install_line(Address address, const CacheLine& line, MESIState state);
    CacheLine* find_line(Address address); 
    const CacheLine* find_line(Address address) const; // used by find_line() and contains()
    std::uint8_t read(Address address) const;
    void write(Address address, std::uint8_t data);
    void handle_read_hit(const CacheLine& line) const;
    void handle_write_hit(CacheLine& line);
    void snoop(Transaction transaction_type, Address address);
    AgentId agent_id() const {return m_agent_id;}

private:
    SystemConfig m_config{};
    std::vector<std::vector<CacheLine>> m_sets{};
    AgentId m_agent_id{};

    std::uint32_t m_offset_bits{};
    std::uint32_t m_set_bits{};

};