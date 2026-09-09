#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

int main()
{
    // Check underlying type sizes
    static_assert(sizeof(Address) == sizeof(std::uint32_t));
    static_assert(sizeof(AgentId) == sizeof(std::uint32_t));
    static_assert(sizeof(TransactionId) == sizeof(std::uint64_t));

    // Check that the types can hold values we expect
    Address address = 0xFFFFFFFF;
    AgentId agent_id = 42;
    TransactionId transaction_id = 123456789ULL;

    assert(address == 0xFFFFFFFF);
    assert(agent_id == 42);
    assert(transaction_id == 123456789ULL);

    // Check AgentType values
    AgentType cpu = AgentType::CPU;
    AgentType dma = AgentType::DMA;
    AgentType accelerator = AgentType::Accelerator;

    assert(cpu != dma);
    assert(cpu != accelerator);
    assert(dma != accelerator);

    // Check MemoryOperation values
    MemoryOperation read = MemoryOperation::Read;
    MemoryOperation write = MemoryOperation::Write;

    assert(read != write);

    std::cout << "All type tests passed.\n";

    return 0;
}

