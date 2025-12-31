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

#include <WebKit/WKImeEvent.h>

namespace WebCore {
class Cursor;
class FloatPoint;
class IntPoint;
class IntRect;
class IntSize;
class Region;
}

namespace WebKit {
class NativeWebKeyboardEvent;
class PlayStationWebView;
struct EditorState;
class LayerTreeContext;
}

namespace API {

class ViewClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    virtual ~ViewClient() = default;

    virtual void setViewNeedsDisplay(WebKit::PlayStationWebView&, const WebCore::Region&) { }
    virtual void enterFullScreen(WebKit::PlayStationWebView&) { }
    virtual void exitFullScreen(WebKit::PlayStationWebView&) { }
    virtual void closeFullScreen(WebKit::PlayStationWebView&) { }
    virtual void beganEnterFullScreen(WebKit::PlayStationWebView&, const WebCore::IntRect&, const WebCore::IntRect&) { }
    virtual void beganExitFullScreen(WebKit::PlayStationWebView&, const WebCore::IntRect&, const WebCore::IntRect&) { }
    virtual void doneWithMouseUpEvent(WebKit::PlayStationWebView&, bool wasEventHandled) { }
    virtual void doneWithKeyboardEvent(WebKit::PlayStationWebView&, const WebKit::NativeWebKeyboardEvent&, bool wasEventHandled) { }
    virtual void didChangeEditorState(WebKit::PlayStationWebView&, WKEditorStateEventType, WebKit::EditorState&) { }
    virtual void didChangeSelectionState(WebKit::PlayStationWebView&, const WTF::String&, const WebCore::IntRect&) { }
    virtual void didChangeCompositionState(WebKit::PlayStationWebView&, const WebCore::IntRect&) { }
    virtual void setCursorPosition(WebKit::PlayStationWebView&, const WebCore::IntPoint&) { }
    virtual void setCursor(WebKit::PlayStationWebView&, const WebCore::Cursor&) { }
    virtual int launchOpenGLServerProcess(WebKit::PlayStationWebView&) { return 0; }
    virtual void enterAcceleratedCompositingMode(WebKit::PlayStationWebView&, const WebKit::LayerTreeContext&) { }
    virtual void exitAcceleratedCompositingMode(WebKit::PlayStationWebView&) { }
    virtual void didChangeContentsVisibility(WebKit::PlayStationWebView&, const WebCore::IntSize&, const WebCore::FloatPoint& position, float scale) { }
};

} // namespace API
