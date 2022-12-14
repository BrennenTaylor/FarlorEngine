#pragma once

#include "BaseAllocator.h"

#include <cstdint>

namespace Farlor {

// Intended to be used per subsystem to access the same memory block
class ProxyAllocator : public Allocator
{
    ProxyAllocator(Allocator& allocator);
    ~ProxyAllocator();

    virtual void* Allocate(size_t size, uint8_t alignment) override;
    virtual void Deallocate(void* pMemory) override;

private:
    ProxyAllocator(const ProxyAllocator&); //Prevent copies because it might cause errors
    ProxyAllocator& operator=(const ProxyAllocator&);

    Allocator& m_allocator;
};

}