/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
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

#include <WebKit/WKAXObject.h>
#include <WebKit/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*WKViewAccessibilityCallback)(WKViewRef view, WKAXObjectRef axObject, WKAXNotification notification, const void* clientInfo);
typedef void(*WKViewAccessibilityTextChangeCallback)(WKViewRef view, WKAXObjectRef axObject, WKAXTextChange textChange, uint32_t offset, WKStringRef text, const void* clientInfo);
typedef void(*WKViewAccessibilityLoadingEventCallback)(WKViewRef view, WKAXObjectRef axObject, WKAXLoadingEvent loadingEvent, const void* clientInfo);
typedef void(*WKViewAccessibilityRootObjectCallback)(WKViewRef view, WKAXObjectRef axObject, const void* clientInfo);
typedef void(*WKViewAccessibilityFocusedObjectCallback)(WKViewRef view, WKAXObjectRef axObject, const void* clientInfo);
typedef void(*WKViewAccessibilityHitTestCallback)(WKViewRef view, WKAXObjectRef axObject, const void* clientInfo);

typedef struct WKViewAccessibilityClientBase {
    int version;
    const void* clientInfo;
} WKViewAccessibilityClientBase;

typedef struct WKViewAccessibilityClientV0 {
    WKViewAccessibilityClientBase                                       base;
    WKViewAccessibilityCallback                                         notification;
    WKViewAccessibilityTextChangeCallback                               textChanged;
    WKViewAccessibilityLoadingEventCallback                             loadingEvent;
    WKViewAccessibilityRootObjectCallback                               rootObject;
    WKViewAccessibilityFocusedObjectCallback                            focusedObject;
    WKViewAccessibilityHitTestCallback                                  hitTest;
} WKViewAccessibilityClientV0;

// accessibility api
WK_EXPORT void WKViewSetViewAccessibilityClient(WKViewRef view, const WKViewAccessibilityClientBase* client);
WK_EXPORT void WKViewAccessibilityRootObject(WKViewRef view);
WK_EXPORT void WKViewAccessibilityFocusedObject(WKViewRef view);
WK_EXPORT void WKViewAccessibilityHitTest(WKViewRef view, const WKPoint& point);

#ifdef __cplusplus
}
#endif
