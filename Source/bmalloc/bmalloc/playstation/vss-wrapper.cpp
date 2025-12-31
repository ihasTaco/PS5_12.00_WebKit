/*
 * Copyright (C) 2024 SONY Interactive Entertainment Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vss-wrapper.h"

#include <memory-extra/vss.h>

extern void* memoryextra_vss_reserve(size_t size, size_t alignment)
{
    return memory_extra::vss::reserve(size, alignment);
}
extern bool memoryextra_vss_release(void* addr, size_t size)
{
    return memory_extra::vss::release(addr, size);
}

extern bool memoryextra_vss_commit(void* addr, size_t size, bool writable, int usage)
{
    return memory_extra::vss::commit(addr, size, writable, usage);
}
extern bool memoryextra_vss_decommit(void* addr, size_t size)
{
    return memory_extra::vss::decommit(addr, size);
}

extern bool memoryextra_vss_protect(void* addr, size_t size, bool readable, bool writable, int usage)
{
    return memory_extra::vss::protect(addr, size, readable, writable, usage);
}
