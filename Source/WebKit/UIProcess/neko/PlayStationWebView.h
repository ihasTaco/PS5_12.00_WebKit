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

#include "APIObject.h"
#include "APIViewClient.h"
#include "APIViewPopupMenuClient.h"
#include "PageClientImpl.h"
#include "WKView.h"
#include "WebPageProxy.h"
#include "WebViewAccessibilityClient.h"

namespace WebKit {

class WebPopupMenuProxyPlayStation;
struct WebPopupItem;

class PlayStationWebView : public API::ObjectImpl<API::Object::Type::View> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static RefPtr<PlayStationWebView> create(const API::PageConfiguration&);
    virtual ~PlayStationWebView();

    void setClient(std::unique_ptr<API::ViewClient>&&);
    void setPopupMenuClient(std::unique_ptr<API::ViewPopupMenuClient>&&);

    WebPageProxy* page() { return m_page.get(); }

    void setViewSize(WebCore::IntSize);
    WebCore::IntSize viewSize() const { return m_viewSize; }

    void setViewState(OptionSet<WebCore::ActivityState>);
    OptionSet<WebCore::ActivityState> viewState() const { return m_viewStateFlags; }

    void showPopupMenu(WebPopupMenuProxyPlayStation*, const Vector<WebPopupItem>&, const WebCore::IntRect&, int32_t);
    void hidePopupMenu();
    void valueChangedForPopupMenu(int);

#if ENABLE(FULLSCREEN_API)
    void willEnterFullScreen();
    void didEnterFullScreen();
    void willExitFullScreen();
    void didExitFullScreen();
    void requestExitFullScreen();
#endif

#if ENABLE(ACCESSIBILITY)
    void setAccessibilityClient(const WKViewAccessibilityClientBase*);
    void handleAccessibilityNotification(WebAccessibilityObject*, WebCore::AXObjectCache::AXNotification);
    void handleAccessibilityTextChange(WebAccessibilityObject*, WebCore::AXTextChange, uint32_t, const String&);
    void handleAccessibilityLoadingEvent(WebAccessibilityObject*, WebCore::AXObjectCache::AXLoadingEvent);
    void accessibilityRootObject();
    void accessibilityFocusedObject();
    void accessibilityHitTest(const WebCore::IntPoint&);
    void handleAccessibilityRootObject(WebAccessibilityObject* axObject);
    void handleAccessibilityFocusedObject(WebAccessibilityObject* axObject);
    void handleAccessibilityHitTest(WebAccessibilityObject* axObject);
#endif
    void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool wasEventHandled);
    void doneWithMouseUpEvent(bool wasEventHandled);
    void didChangeEditorState(const WebKit::EditorState&);
    void didChangeSelectionState(const String& selectedText, const WebCore::IntRect& selectedRect);
    void didChangeCompositionState(const WebCore::IntRect& compositionRect);

#if ENABLE(PUNCH_HOLE)
    void getPunchHoles(WKPunchHole* holes, int arrayLengthOfHoles, int* numberOfCopiedPunchHoles, int* numberOfActualPunchHoles);
#endif

    // Functions called by PageClientImpl
    void setViewNeedsDisplay(const WebCore::Region&);
#if ENABLE(FULLSCREEN_API)
    bool isFullScreen();
    void closeFullScreenManager();
    void enterFullScreen();
    void exitFullScreen();
    void beganEnterFullScreen(const WebCore::IntRect&, const WebCore::IntRect&);
    void beganExitFullScreen(const WebCore::IntRect&, const WebCore::IntRect&);
#endif
    void setCursor(const WebCore::Cursor&);
#if ENABLE(CURSOR_NAVIGATION)
    void setCursorPosition(const WebCore::IntPoint&);
#endif
    void enterAcceleratedCompositingMode(const LayerTreeContext&);
    void exitAcceleratedCompositingMode();
    void updateAcceleratedCompositingMode(const LayerTreeContext&);
    int launchOpenGLServerProcess();
    void setUsesOffscreenRendering(bool);
    bool usesOffscreenRendering() const;

private:
    PlayStationWebView(const API::PageConfiguration&);

    std::unique_ptr<API::ViewClient> m_client;
    std::unique_ptr<API::ViewPopupMenuClient> m_popupMenuClient;
    std::unique_ptr<WebKit::PageClientImpl> m_pageClient;
    RefPtr<WebPageProxy> m_page;
    OptionSet<WebCore::ActivityState> m_viewStateFlags;
    bool m_usesOffscreenRendering { false };

    WebCore::IntSize m_viewSize = { };

#if ENABLE(FULLSCREEN_API)
    bool m_isFullScreen { false };
#endif

#if ENABLE(ACCESSIBILITY)
    WebViewAccessibilityClient m_axClient;
#endif

    WebKit::EditorState m_editorState;

    RefPtr<WebPopupMenuProxyPlayStation> m_popupMenuProxy;
};

} // namespace WebKit
