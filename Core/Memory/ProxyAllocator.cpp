#include "ProxyAllocator.h"

#include <cassert>
#include <cstdint>

namespace Farlor {

ProxyAllocator::ProxyAllocator(Allocator& allocator)
    : Allocator(allocator.m_size, allocator.m_pStart)
    , m_allocator(allocator)
{
}

ProxyAllocator::~ProxyAllocator()
{
}

void* ProxyAllocator::Allocate(size_t size, uint8_t alignment)
{
    assert(size != 0);
    m_numAllocations++;

    size_t mem = m_allocator.m_usedMemory;
    void* pMemory = m_allocator.Allocate(size, alignment);
    m_usedMemory += m_allocator.m_usedMemory - mem;
    return pMemory;
}

void ProxyAllocator::Deallocate(void* p)
{
    m_numAllocations--;
    size_t mem = m_allocator.m_usedMemory;
    m_allocator.Deallocate(p);
    m_usedMemory -= mem - m_allocator.m_usedMemory;
}

}