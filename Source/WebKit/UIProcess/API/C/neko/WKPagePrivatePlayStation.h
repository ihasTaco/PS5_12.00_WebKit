/*
 * Copyright (C) 2021 Sony Interactive Entertainment Inc.
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

#pragma once

#include <WebKit/WKBase.h>
#include <WebKit/WKEventPlayStation.h>
#include <WebKit/WKImeEvent.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT void WKPageHandleKeyboardEvent(WKPageRef, WKKeyboardEvent);
WK_EXPORT void WKPageHandleMouseEvent(WKPageRef, WKMouseEvent);
WK_EXPORT void WKPageHandleWheelEvent(WKPageRef, WKWheelEvent);

WK_EXPORT void WKPageBlurFocusedNode(WKPageRef);
WK_EXPORT void WKPageSetCaretVisible(WKPageRef, bool);

WK_EXPORT void WKPageSuspendActiveDOMObjectsAndAnimations(WKPageRef);
WK_EXPORT void WKPageResumeActiveDOMObjectsAndAnimations(WKPageRef);

WK_EXPORT void WKPageHandleImeEvent(WKPageRef, const WKImeEvent*);
WK_EXPORT void WKPageClearSelection(WKPageRef);

WK_EXPORT void WKPagePaint(WKPageRef, unsigned char* cairoSurfaceARGB32Data, WKSize surfaceSize, WKRect paintRect);

WK_EXPORT bool WKPageWebProcessIsResponsive(WKPageRef);

WK_EXPORT void WKPageSetDrawsBackground(WKPageRef, bool drawsBackground);
WK_EXPORT void WKPageScrollBy(WKPageRef, WKPoint);
WK_EXPORT void WKPageSetPageScaleFactor(WKPageRef, double);
WK_EXPORT double WKPageGetPageScaleFactor(WKPageRef);
WK_EXPORT void WKPageSetViewScaleFactor(WKPageRef, double);
WK_EXPORT double WKPageGetViewScaleFactor(WKPageRef);

#ifdef __cplusplus
}
#endif
