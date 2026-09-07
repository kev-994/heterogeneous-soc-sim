#pragma once

#include "bus.hpp"

#include <cstdint>
#include <array>

struct AddressDecomp
{
    std::uint32_t tag{};
    std::uint8_t  set_index{};
    std::uint8_t  offset{};
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
    bool valid{};
    bool dirty{};
    std::uint32_t tag{};
    std::array<std::uint8_t, 16> data{}; // 16 1-byte integers
    MESIState state{}; // eventually replaces valid and dirty but I don't want to break existing implementation
};

class Cache
{
public:
    enum class CacheResult
    {
        Hit,
        Miss
    };

    struct ReadResult
    {
        CacheResult result{};
        std::uint8_t data{};
    };

    struct SnoopResult
    {
        bool has_data{};
        std::array<std::uint8_t, 16> data{};
    };

    ReadResult read(std::uint32_t address);
    void write(std::uint32_t address, std::uint8_t data);
    void write_memory(std::uint32_t address, std::uint8_t data);
    void install_cache_line(CacheLine& line, std::uint32_t line_base, std::uint32_t tag);
    void evict(CacheLine& line, std::uint32_t set);
    
    SnoopResult snoop(BusRequest request, std::uint32_t address);

    MESIState get_state(std::uint32_t address) const;
    friend std::uint8_t getByte(const Cache& cache, std::uint32_t address);

private:
    // 2 sets × 2 ways of CacheLine
    std::array<std::array<CacheLine, 2>, 2> m_cache{};
    std::array<std::uint8_t, 256> m_memory{};
};

AddressDecomp decompose(std::uint32_t address);