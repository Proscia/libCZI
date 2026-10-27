// SPDX-FileCopyrightText: 2017-2022 Carl Zeiss Microscopy GmbH
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "stdAllocator.h"
#include <limits>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "libCZI_Config_Internal.h"

using namespace libCZI::detail;

constexpr int ALLOC_ALIGNMENT = 32;

void* CHeapAllocator::Allocate(std::uint64_t size)
{
    if (size > (std::numeric_limits<size_t>::max)())
    {
        throw std::out_of_range("The requested size for allocation is out-of-range.");
    }

#if LIBCZI_HAVE_ALIGNED_ALLOC
    // Specification requires that 'size' is an integral multiple of 'alignment' (https://en.cppreference.com/w/cpp/memory/c/aligned_alloc),
    // so we round up to the next multiple of 'alignment' here.
    const size_t actual_size = ((size_t)size + ALLOC_ALIGNMENT - 1) / ALLOC_ALIGNMENT * ALLOC_ALIGNMENT;
    void* ptr = aligned_alloc(ALLOC_ALIGNMENT, actual_size);
#elif  LIBCZI_HAVE__ALIGNED_MALLOC
    const size_t actual_size = (size_t)size;
    void* ptr = _aligned_malloc(actual_size, ALLOC_ALIGNMENT);
#else
    void* p1;
    void** p2;
    int offset = ALLOC_ALIGNMENT - 1 + sizeof(void*);
    const size_t actual_size = (size_t)size;
    p1 = malloc(actual_size + offset);
    if (p1 == nullptr)
    {
        return nullptr;
    }

    p2 = (void**)(((size_t)(p1)+offset) & ~(size_t)(ALLOC_ALIGNMENT - 1));
    p2[-1] = p1;
    void* ptr = p2;
#endif

    // Zero-initialize allocated memory to prevent undefined behavior from stale
    // heap contents being consumed by downstream bitmap operations.
    // Originally reported upstream in zeiss-microscopy/libCZI#40 (Nov 2018) and
    // closed there without a fix; ported from Proscia fork commit afd2c65.
    if (ptr != nullptr)
    {
        memset(ptr, 0, actual_size);
    }

    return ptr;
}

void CHeapAllocator::Free(void* ptr)
{
#if LIBCZI_HAVE_ALIGNED_ALLOC // aligned_alloc goes together with 'free'
    free(ptr);
#elif LIBCZI_HAVE__ALIGNED_MALLOC
    _aligned_free(ptr);
#else
    void* p1 = ((void**)ptr)[-1];         // get the pointer to the buffer we allocated
    free(p1);
#endif
}
