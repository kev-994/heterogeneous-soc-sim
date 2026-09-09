#include "config.hpp"

#include <cassert>
#include <iostream>

int main()
{
    // Test default configuration
    SystemConfig config{};

    assert(config.address_width == 32);
    assert(config.memory_capacity == 256);

    assert(config.cpu_count == 2);
    assert(config.dma_count == 1);
    assert(config.accelerator_count == 1);

    assert(config.cache_line_size == 16);
    assert(config.cache_set_count == 2);
    assert(config.cache_associativity == 2);

    assert(config.simulation_mode == SimulationMode::Functional);

    // Test that configuration can be changed
    SystemConfig custom_config{};

    custom_config.address_width = 64;
    custom_config.memory_capacity = 4096;

    custom_config.cpu_count = 4;
    custom_config.dma_count = 2;
    custom_config.accelerator_count = 3;

    custom_config.cache_line_size = 64;
    custom_config.cache_set_count = 16;
    custom_config.cache_associativity = 4;

    custom_config.simulation_mode = SimulationMode::Timing;

    assert(custom_config.address_width == 64);
    assert(custom_config.memory_capacity == 4096);

    assert(custom_config.cpu_count == 4);
    assert(custom_config.dma_count == 2);
    assert(custom_config.accelerator_count == 3);

    assert(custom_config.cache_line_size == 64);
    assert(custom_config.cache_set_count == 16);
    assert(custom_config.cache_associativity == 4);

    assert(custom_config.simulation_mode == SimulationMode::Timing);

    std::cout << "All config tests passed.\n";

    return 0;
}

