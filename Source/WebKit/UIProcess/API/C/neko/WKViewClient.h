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
#include <WebKit/WKGeometry.h>
#include <WebKit/WKImeEvent.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kWKCursorTypePointer = 0,
    kWKCursorTypeCross = 1,
    kWKCursorTypeHand = 2,
    kWKCursorTypeIBeam = 3,
    kWKCursorTypeWait = 4,
    kWKCursorTypeHelp = 5,
    kWKCursorTypeEastResize = 6,
    kWKCursorTypeNorthResize = 7,
    kWKCursorTypeNorthEastResize = 8,
    kWKCursorTypeNorthWestResize = 9,
    kWKCursorTypeSouthResize = 10,
    kWKCursorTypeSouthEastResize = 11,
    kWKCursorTypeSouthWestResize = 12,
    kWKCursorTypeWestResize = 13,
    kWKCursorTypeNorthSouthResize = 14,
    kWKCursorTypeEastWestResize = 15,
    kWKCursorTypeNorthEastSouthWestResize = 16,
    kWKCursorTypeNorthWestSouthEastResize = 17,
    kWKCursorTypeColumnResize = 18,
    kWKCursorTypeRowResize = 19,
    kWKCursorTypeMiddlePanning = 20,
    kWKCursorTypeEastPanning = 21,
    kWKCursorTypeNorthPanning = 22,
    kWKCursorTypeNorthEastPanning = 23,
    kWKCursorTypeNorthWestPanning = 24,
    kWKCursorTypeSouthPanning = 25,
    kWKCursorTypeSouthEastPanning = 26,
    kWKCursorTypeSouthWestPanning = 27,
    kWKCursorTypeWestPanning = 28,
    kWKCursorTypeMove = 29,
    kWKCursorTypeVerticalText = 30,
    kWKCursorTypeCell = 31,
    kWKCursorTypeContextMenu = 32,
    kWKCursorTypeAlias = 33,
    kWKCursorTypeProgress = 34,
    kWKCursorTypeNoDrop = 35,
    kWKCursorTypeCopy = 36,
    kWKCursorTypeNone = 37,
    kWKCursorTypeNotAllowed = 38,
    kWKCursorTypeZoomIn = 39,
    kWKCursorTypeZoomOut = 40,
    kWKCursorTypeGrab = 41,
    kWKCursorTypeGrabbing = 42,
    kWKCursorTypeCustom = 43
};
typedef uint32_t WKCursorType;

typedef void(*WKViewClientCallback)(WKViewRef view, const void* clientInfo);

typedef void(*WKViewSetCursorPositionCallback)(WKViewRef view, WKPoint cursorPosition, const void* clientInfo);
typedef void(*WKViewSetCursorCallback)(WKViewRef view, WKCursorType cursorType, const void* clientInfo);
typedef void(*WKViewSetViewNeedsDisplay)(WKViewRef view, WKRect rect, const void* clientInfo);
typedef void(*WKViewDoneWithMouseEventCallback)(WKViewRef view, bool wasEventHandled, const void* clientInfo);
typedef void(*WKViewDoneWithKeyEventCallback)(WKViewRef view, WKKeyboardEvent event, bool wasEventHandled, const void* clientInfo);

// IME callback
typedef void(*WKViewDidChangeEditorStateCallback)(WKViewRef view, WKEditorState editorState, WKRect inputRect, WKStringRef inputText, const void* clientInfo);
typedef void(*WKViewDidChangeSelectionStateCallback)(WKViewRef view, WKRect selectedRect, WKStringRef selectedText, const void* clientInfo);
typedef void(*WKViewDidChangeCompositionStateCallback)(WKViewRef view, WKRect compositionRect, const void* clientInfo);

// fullscreen callback
typedef void(*WKViewEnterFullScreen)(WKViewRef view, const void* clientInfo);
typedef void(*WKViewExitFullScreen)(WKViewRef view, const void* clientInfo);
typedef void(*WKViewCloseFullScreen)(WKViewRef view, const void* clientInfo);
typedef void(*WKViewBeganEnterFullScreen)(WKViewRef view, const WKRect initialFrame, const WKRect finalFrame, const void* clientInfo);
typedef void(*WKViewBeganExitFullScreen)(WKViewRef view, const WKRect initialFrame, const WKRect finalFrame, const void* clientInfo);

// Accelerated compositing mode callback
typedef void(*WKViewEnterAcceleratedCompositingMode)(WKViewRef page, uint32_t canvasHandle, const void* clientInfo);
typedef void(*WKViewExitAcceleratedCompositingMode)(WKViewRef page, const void* clientInfo);
typedef int (*WKViewLaunchOpenGLServerProcess)(WKViewRef page, const void* clientInfo);

// CoordinatedGraphics callback
typedef void (*WKViewDidChangeContentsVisibilityCallback)(WKViewRef view, WKSize size, WKPoint position, float scale, const void* clientInfo);

// PunchHole Compositing API
struct WKPunchHole {
    float quad[4][2];
    float opacity;
    uint32_t canvasHandle;
    uint32_t flags;
};
typedef struct WKPunchHole WKPunchHole;

typedef struct WKViewClientBase {
    int version;
    const void* clientInfo;
} WKViewClientBase;

typedef struct WKViewClientV0 {
    WKViewClientBase base;

    // version 0
    WKViewSetViewNeedsDisplay setViewNeedsDisplay;
    WKViewEnterFullScreen enterFullScreen;
    WKViewExitFullScreen exitFullScreen;
    WKViewCloseFullScreen closeFullScreen;
    WKViewBeganEnterFullScreen beganEnterFullScreen;
    WKViewBeganExitFullScreen beganExitFullScreen;
    WKViewDoneWithMouseEventCallback doneWithMouseUpEvent;
    WKViewDoneWithKeyEventCallback doneWithKeyboardEvent;
    WKViewDidChangeEditorStateCallback didChangeEditorState;
    WKViewDidChangeSelectionStateCallback didChangeSelectionState;
    WKViewDidChangeCompositionStateCallback didChangeCompositionState;
    WKViewSetCursorPositionCallback setCursorPosition;
    WKViewSetCursorCallback setCursor;
    WKViewEnterAcceleratedCompositingMode enterAcceleratedCompositingMode;
    WKViewExitAcceleratedCompositingMode exitAcceleratedCompositingMode;
    WKViewDidChangeContentsVisibilityCallback didChangeContentsVisibility;
} WKViewClientV0;

typedef struct WKViewClientV1 {
    WKViewClientBase base;

    // version 0
    WKViewSetViewNeedsDisplay setViewNeedsDisplay;
    WKViewEnterFullScreen enterFullScreen;
    WKViewExitFullScreen exitFullScreen;
    WKViewCloseFullScreen closeFullScreen;
    WKViewBeganEnterFullScreen beganEnterFullScreen;
    WKViewBeganExitFullScreen beganExitFullScreen;
    WKViewDoneWithMouseEventCallback doneWithMouseUpEvent;
    WKViewDoneWithKeyEventCallback doneWithKeyboardEvent;
    WKViewDidChangeEditorStateCallback didChangeEditorState;
    WKViewDidChangeSelectionStateCallback didChangeSelectionState;
    WKViewDidChangeCompositionStateCallback didChangeCompositionState;
    WKViewSetCursorPositionCallback setCursorPosition;
    WKViewSetCursorCallback setCursor;
    WKViewEnterAcceleratedCompositingMode enterAcceleratedCompositingMode;
    WKViewExitAcceleratedCompositingMode exitAcceleratedCompositingMode;
    WKViewDidChangeContentsVisibilityCallback didChangeContentsVisibility;

    // version 1
    WKViewLaunchOpenGLServerProcess launchOpenGLServerProcess;
} WKViewClientV1;

#ifdef __cplusplus
}
#endif
