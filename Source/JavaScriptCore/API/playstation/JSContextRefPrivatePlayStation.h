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

#ifndef JSContextRefPrivatePlayStation_h
#define JSContextRefPrivatePlayStation_h

#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSContextRefPrivate.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
@function
@abstract Sets the run loop used by the Web Inspector debugger when evaluating JavaScript in this context with current thread's run loop.
@param ctx The JSGlobalContext that you want to change.
*/
JS_EXPORT void JSGlobalContextSetDebuggerRunLoopWithCurrentRunLoop(JSGlobalContextRef ctx);

/*!
@function
@abstract Unsets the run loop used by the Web Inspector debugger when evaluating JavaScript in this context.
@param ctx The JSGlobalContext that you want to change.
*/
JS_EXPORT void JSGlobalContextUnsetDebuggerRunLoop(JSGlobalContextRef ctx);

/*!
@function
@abstract Iterates current thread's run loop.
*/
JS_EXPORT void JSRunLoopIterateCurrentRunLoop(void);

/*!
@function
@abstract To unload JSC module, it is required to call this function to terminate background threads.
*/
JS_EXPORT void JSShutdown(void);

#define JSC_HAS_JSShutdown 1

#ifdef __cplusplus
}
#endif

#endif /* JSContextRefPrivate_h */
