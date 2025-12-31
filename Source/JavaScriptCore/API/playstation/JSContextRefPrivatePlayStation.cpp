/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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

#include "config.h"
#include "JSContextRefPrivatePlayStation.h"
#include <bmalloc/bmalloc.h>

#if ENABLE(REMOTE_INSPECTOR)
#include "APICast.h"
#include "JSAPIGlobalObject.h"
#include "JSGlobalObjectDebuggable.h"
#include <wtf/RunLoop.h>
#endif

void JSRunLoopIterateCurrentRunLoop(void)
{
    RunLoop::current().cycle();
}

void JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(JSGlobalContextRef ctx)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSC::JSGlobalObject* globalObject = toJS(ctx);
    JSC::VM& vm = globalObject->vm();
    JSC::JSLockHolder lock(vm);

    globalObject->inspectorDebuggable().setTargetFunctionDispatcher(&RunLoop::current());
#else
    UNUSED_PARAM(ctx);
#endif
}

void JSGlobalContextUnsetDebuggerRunLoop(JSGlobalContextRef ctx)
{
#if ENABLE(REMOTE_INSPECTOR)
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }

    JSC::JSGlobalObject* globalObject = toJS(ctx);
    JSC::VM& vm = globalObject->vm();
    JSC::JSLockHolder lock(vm);

    globalObject->inspectorDebuggable().setTargetFunctionDispatcher(nullptr);
#else
    UNUSED_PARAM(ctx);
#endif
}

void JSShutdown(void)
{
    bmalloc::api::disableScavenger();
}
