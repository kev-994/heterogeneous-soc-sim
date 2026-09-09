#pragma once

#include <bit>
#include <concepts>
#include <cstdint>

template <std::unsigned_integral T>
[[nodiscard]] constexpr int clog2(T val) noexcept { 
    return (val <= 1) ? 0 : std::bit_width(static_cast<T>(val - 1));
}

constexpr bool is_power_of_two(std::uint32_t value) noexcept
{
    return value != 0 && (value & (value - 1)) == 0;
}