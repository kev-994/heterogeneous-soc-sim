#include "utils.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

void test_clog2_powers_of_two()
{
    assert(clog2(1u) == 0);
    assert(clog2(2u) == 1);
    assert(clog2(4u) == 2);
    assert(clog2(8u) == 3);
    assert(clog2(16u) == 4);
    assert(clog2(32u) == 5);
}

void test_clog2_non_powers_of_two()
{
    assert(clog2(3u) == 2);
    assert(clog2(5u) == 3);
    assert(clog2(6u) == 3);
    assert(clog2(7u) == 3);
    assert(clog2(9u) == 4);
    assert(clog2(15u) == 4);
    assert(clog2(17u) == 5);
}

void test_clog2_zero()
{
    assert(clog2(0u) == 0);
}

void test_clog2_different_unsigned_types()
{
    assert(clog2(std::uint8_t{16}) == 4);
    assert(clog2(std::uint16_t{256}) == 8);
    assert(clog2(std::uint32_t{1024}) == 10);
    assert(clog2(std::uint64_t{4096}) == 12);
}

int main()
{
    test_clog2_powers_of_two();
    test_clog2_non_powers_of_two();
    test_clog2_zero();
    test_clog2_different_unsigned_types();

    std::cout << "All utils tests passed!\n";

    return 0;
}