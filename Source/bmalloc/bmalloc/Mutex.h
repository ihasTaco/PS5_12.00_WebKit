/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2018 Yusuke Suzuki <utatane.tea@gmail.com>.
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

#pragma once

#include "BAssert.h"
#include <atomic>
#include <mutex>
#include <thread>

#if BUSE(PTHREAD_MUTEX)
#include <pthread.h>
#if BPLATFORM(PLAYSTATION)
#include <pthread_np.h>
#endif
#endif

// A fast replacement for std::mutex.

namespace bmalloc {

class Mutex;

using UniqueLockHolder = std::unique_lock<Mutex>;
using LockHolder = std::scoped_lock<Mutex>;

class Mutex {
public:
#if BUSE(PTHREAD_MUTEX)
    Mutex();
    ~Mutex();
#else
    constexpr Mutex() = default;
#endif

    void lock();
#if !BUSE(PTHREAD_MUTEX)
    bool try_lock();
#endif
    void unlock();

private:
#if BUSE(PTHREAD_MUTEX)
    pthread_mutex_t m_mutex { PTHREAD_MUTEX_INITIALIZER };
#else
    BEXPORT void lockSlowCase();

    std::atomic<bool> m_flag { false };
    std::atomic<bool> m_isSpinning { false };
#endif
};

static inline void sleep(
    UniqueLockHolder& lock, std::chrono::milliseconds duration)
{
    if (duration == std::chrono::milliseconds(0))
        return;
    
    lock.unlock();
    std::this_thread::sleep_for(duration);
    lock.lock();
}

static inline void waitUntilFalse(
    UniqueLockHolder& lock, std::chrono::milliseconds sleepDuration,
    bool& condition)
{
    while (condition) {
        condition = false;
        sleep(lock, sleepDuration);
    }
}

#if BUSE(PTHREAD_MUTEX)
inline Mutex::Mutex()
{
    pthread_mutex_init(&m_mutex, nullptr);
#if BPLATFORM(PLAYSTATION)
    pthread_mutex_setname_np(&m_mutex, "SceNKBMalloc");
#endif
}

inline Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mutex);
}

inline void Mutex::lock()
{
    pthread_mutex_lock(&m_mutex);
}

inline void Mutex::unlock()
{
    pthread_mutex_unlock(&m_mutex);
}
#else
inline bool Mutex::try_lock()
{
    return !m_flag.exchange(true, std::memory_order_acquire);
}

inline void Mutex::lock()
{
    if (!try_lock())
        lockSlowCase();
}

inline void Mutex::unlock()
{
    m_flag.store(false, std::memory_order_release);
}
#endif

} // namespace bmalloc
