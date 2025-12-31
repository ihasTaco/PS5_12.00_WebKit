/*
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "LargeMap.h"
#include <utility>

#if !BUSE(LIBPAS)

namespace bmalloc {

LargeRange LargeMap::remove(size_t alignment, size_t size)
{
    size_t alignmentMask = alignment - 1;

    LargeRange* candidate = m_free.end();
    for (LargeRange* it = m_free.begin(); it != m_free.end(); ++it) {
        if (!it->isEligibile())
            continue;

        if (it->size() < size)
            continue;

        if (candidate != m_free.end() && candidate->begin() < it->begin())
            continue;

        if (test(it->begin(), alignmentMask)) {
            char* aligned = roundUpToMultipleOf(alignment, it->begin());
            if (aligned < it->begin()) // Check for overflow.
                continue;

            char* alignedEnd = aligned + size;
            if (alignedEnd < aligned) // Check for overflow.
                continue;

            if (alignedEnd > it->end())
                continue;
        }

        candidate = it;
    }
    
    if (candidate == m_free.end())
        return LargeRange();

    return m_free.pop(candidate);
}

void LargeMap::add(const LargeRange& range)
{
    LargeRange merged = range;

    for (size_t i = 0; i < m_free.size(); ++i) {
        if (!canMerge(merged, m_free[i]))
            continue;

        merged = merge(merged, m_free.pop(i--));
    }

    merged.setUsedSinceLastScavenge();
    m_free.push(merged);
}

void LargeMap::markAllAsEligibile()
{
    for (LargeRange& range : m_free)
        range.setEligible(true);
}

#if BPLATFORM(PLAYSTATION)

void LargeMap::purge()
{
#if !defined(NDEBUG)
    size_t beforeSize = 0;
    size_t purged = 0;
    for (auto& range : m_free)
        beforeSize += range.size();
#endif
    for (size_t index = 0; index < m_free.size();) {
        const LargeRange& range = m_free[index];

        char* begin = roundUpToMultipleOf(vmPageSizePhysical(), range.begin());
        char* end = roundDownToMultipleOf(vmPageSizePhysical(), range.end());
        BASSERT(begin <= end);

        size_t size = end - begin;
        BASSERT(size <= range.size());
        if (!size) {
            index++;
            continue;
        }

        bool matchBegin = (begin == range.begin());
        bool matchEnd = (end == range.end());

        if (matchBegin && matchEnd)
            m_free.pop(index);
        else if (matchBegin)
            m_free[index++] = range.split(size).second;
        else if (matchEnd)
            m_free[index++] = range.split(range.size() - size).first;
        else {
            auto [left, right] = range.split(begin - range.begin());
            m_free[index++] = left;
            m_free.insert(index++, right.split(size).second);
        }
        vmDeallocate(begin, size);

#if !defined(NDEBUG)
        purged += size;
#endif
    }

#if !defined(NDEBUG)
    size_t afterSize = 0;
    for (auto& range : m_free)
        afterSize += range.size();
    BASSERT(beforeSize == (afterSize + purged));
#endif
}
#endif

} // namespace bmalloc

#endif
