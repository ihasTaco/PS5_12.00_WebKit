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

#include "config.h"
#include "WKView.h"

#include "APIArray.h"
#include "APIClient.h"
#include "APIPageConfiguration.h"
#include "APIViewClient.h"
#include "APIViewPopupMenuClient.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "PlayStationWebView.h"
#include "WKAPICast.h"
#include "WKSharedAPICast.h"
#include "WebEventFactory.h"
#include "WebPopupMenuItem.h"
#include "WebPopupMenuProxyPlayStation.h"
#include <WKPagePrivatePlayStation.h>
#include <WKViewAccessibilityClient.h>
#include <WTF/WallTime.h>
#include <WebCore/Cursor.h>
#include <WebCore/Region.h>

namespace API {
template<> struct ClientTraits<WKViewClientBase> {
    typedef std::tuple<WKViewClientV0, WKViewClientV1> Versions;
};
template<> struct ClientTraits<WKViewPopupMenuClientBase> {
    typedef std::tuple<WKViewPopupMenuClientV0> Versions;
};
}

WebCore::FloatSize toFloatSize(WKSize size)
{
    return WebCore::FloatSize(size.width, size.height);
}

WebKit::WebEventType toWebEventType(WKEventType type)
{
    return static_cast<WebKit::WebEventType>(type);
}

WebKit::WebMouseEventButton toWebMouseEventButton(WKEventMouseButton button)
{
    return static_cast<WebKit::WebMouseEventButton>(button);
}

WKEventType toWKEventType(WebKit::WebEventType type)
{
    return static_cast<WKEventType>(type);
}

WKKeyboardEvent toWKKeyboardEvent(const WebKit::NativeWebKeyboardEvent& event)
{
    WKKeyboardEvent keyboardEvent = { };
    keyboardEvent.type = toWKEventType(event.type());
    keyboardEvent.virtualKeyCode = event.nativeVirtualKeyCode();
    keyboardEvent.modifiers = WebKit::toAPI(event.modifiers());
    return keyboardEvent;
}

WKCursorType toWKCursorType(const WebCore::Cursor& cursor)
{
    switch (cursor.type()) {
    case WebCore::Cursor::Hand:
        return kWKCursorTypeHand;
    case WebCore::Cursor::None:
        return kWKCursorTypeNone;
    case WebCore::Cursor::Pointer:
    default:
        return kWKCursorTypePointer;
    }
}

WKViewRef WKViewCreate(WKPageConfigurationRef configuration)
{
    return WebKit::toAPI(WebKit::PlayStationWebView::create(*WebKit::toImpl(configuration)).leakRef());
}

WKPageRef WKViewGetPage(WKViewRef view)
{
    return WebKit::toAPI(WebKit::toImpl(view)->page());
}

void WKViewSetSize(WKViewRef view, WKSize viewSize)
{
    WebKit::toImpl(view)->setViewSize(WebKit::toIntSize(viewSize));
}

static void setViewActivityStateFlag(WKViewRef view, WebCore::ActivityState flag, bool set)
{
    auto viewState = WebKit::toImpl(view)->viewState();
    if (set)
        viewState.add(flag);
    else
        viewState.remove(flag);
    WebKit::toImpl(view)->setViewState(viewState);
}

void WKViewSetFocus(WKViewRef view, bool focused)
{
    setViewActivityStateFlag(view, WebCore::ActivityState::IsFocused, focused);
}

void WKViewSetActive(WKViewRef view, bool active)
{
    setViewActivityStateFlag(view, WebCore::ActivityState::WindowIsActive, active);
}

void WKViewSetVisible(WKViewRef view, bool visible)
{
    setViewActivityStateFlag(view, WebCore::ActivityState::IsVisible, visible);
}

void WKViewWillEnterFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    WebKit::toImpl(view)->willEnterFullScreen();
#endif
}

void WKViewDidEnterFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    WebKit::toImpl(view)->didEnterFullScreen();
#endif
}

void WKViewWillExitFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    WebKit::toImpl(view)->willExitFullScreen();
#endif
}

void WKViewDidExitFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    WebKit::toImpl(view)->didExitFullScreen();
#endif
}

void WKViewRequestExitFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    WebKit::toImpl(view)->requestExitFullScreen();
#endif
}

bool WKViewIsFullScreen(WKViewRef view)
{
#if ENABLE(FULLSCREEN_API)
    return WebKit::toImpl(view)->isFullScreen();
#else
    return false;
#endif
}

void WKViewValueChangedForPopupMenu(WKViewRef view, int selectedIndex)
{
    WebKit::toImpl(view)->valueChangedForPopupMenu(selectedIndex);
}

// accessibility api
void WKViewSetViewAccessibilityClient(WKViewRef view, const WKViewAccessibilityClientBase* client)
{
#if ENABLE(ACCESSIBILITY)
    WebKit::toImpl(view)->setAccessibilityClient(client);
#endif
}

void WKViewAccessibilityRootObject(WKViewRef view)
{
#if ENABLE(ACCESSIBILITY)
    WebKit::toImpl(view)->accessibilityRootObject();
#endif
}

void WKViewAccessibilityFocusedObject(WKViewRef view)
{
#if ENABLE(ACCESSIBILITY)
    WebKit::toImpl(view)->accessibilityFocusedObject();
#endif
}

void WKViewAccessibilityHitTest(WKViewRef view, const WKPoint& point)
{
#if ENABLE(ACCESSIBILITY)
    return WebKit::toImpl(view)->accessibilityHitTest(WebKit::toIntPoint(point));
#endif
}

void WKViewGetPunchHoles(WKViewRef view, WKPunchHole* holes, int arrayLengthOfHoles, int* numberOfCopiedPunchHoles, int* numberOfActualPunchHoles)
{
    ASSERT(holes);
    ASSERT(numberOfCopiedPunchHoles);
    ASSERT(numberOfActualPunchHoles);
#if ENABLE(PUNCH_HOLE)
    WebKit::toImpl(view)->getPunchHoles(holes, arrayLengthOfHoles, numberOfCopiedPunchHoles, numberOfActualPunchHoles);
#else
    *numberOfCopiedPunchHoles = 0;
    *numberOfActualPunchHoles = 0;
#endif
}

void WKViewSetViewClient(WKViewRef view, const WKViewClientBase* client)
{
    class ViewClient final : public API::Client<WKViewClientBase>, public API::ViewClient {
    public:
        explicit ViewClient(const WKViewClientBase* client)
        {
            initialize(client);
        }

    private:
        void setViewNeedsDisplay(WebKit::PlayStationWebView& view, const WebCore::Region& region) final
        {
            if (!m_client.setViewNeedsDisplay)
                return;
            m_client.setViewNeedsDisplay(WebKit::toAPI(&view), WebKit::toAPI(region.bounds()), m_client.base.clientInfo);
        }

        void enterFullScreen(WebKit::PlayStationWebView& view)
        {
            if (!m_client.enterFullScreen)
                return;
            m_client.enterFullScreen(WebKit::toAPI(&view), m_client.base.clientInfo);
        }

        void exitFullScreen(WebKit::PlayStationWebView& view)
        {
            if (!m_client.exitFullScreen) {
                view.willExitFullScreen();
                return;
            }
            m_client.exitFullScreen(WebKit::toAPI(&view), m_client.base.clientInfo);
        }

        void closeFullScreen(WebKit::PlayStationWebView& view)
        {
            if (!m_client.closeFullScreen)
                return;
            m_client.closeFullScreen(WebKit::toAPI(&view), m_client.base.clientInfo);
        }

        void beganEnterFullScreen(WebKit::PlayStationWebView& view, const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame)
        {
            if (!m_client.beganEnterFullScreen) {
                view.didEnterFullScreen();
                return;
            }
            m_client.beganEnterFullScreen(WebKit::toAPI(&view), WebKit::toAPI(initialFrame), WebKit::toAPI(finalFrame), m_client.base.clientInfo);
        }

        void beganExitFullScreen(WebKit::PlayStationWebView& view, const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame)
        {
            if (!m_client.beganExitFullScreen) {
                view.didExitFullScreen();
                return;
            }
            m_client.beganExitFullScreen(WebKit::toAPI(&view), WebKit::toAPI(initialFrame), WebKit::toAPI(finalFrame), m_client.base.clientInfo);

        }

        void didChangeEditorState(WebKit::PlayStationWebView& view, WKEditorStateEventType eventType, WebKit::EditorState& editorState)
        {
            if (!m_client.didChangeEditorState)
                return;

            CString psOSKAttr = editorState.psOSKAttr.utf8();

            WKEditorState imeEditorState;
            imeEditorState.eventType = (WKEditorStateEventType)eventType;
            imeEditorState.contentEditable = editorState.isContentEditable;
            imeEditorState.type = (WKInputFieldType)editorState.fieldType;
            imeEditorState.lang = (WKInputLanguage)editorState.fieldLang;
            imeEditorState.caretOffset = editorState.caretOffset;
            imeEditorState.hasPreviousNode = editorState.hasPreviousNode;
            imeEditorState.hasNextNode = editorState.hasNextNode;
            imeEditorState.psOSKAttr = psOSKAttr.data();
            imeEditorState.isInLoginForm = editorState.isInLoginForm;
            imeEditorState.canSubmitImplicitly = editorState.canSubmitImplicitly;
            m_client.didChangeEditorState(WebKit::toAPI(&view), imeEditorState, WebKit::toAPI(editorState.fieldRect), WebKit::toAPI(editorState.fieldText.impl()), m_client.base.clientInfo);
        }

        void didChangeSelectionState(WebKit::PlayStationWebView& view, const String& selectedText, const WebCore::IntRect& selectedRect)
        {
            if (!m_client.didChangeSelectionState)
                return;
            m_client.didChangeSelectionState(WebKit::toAPI(&view), WebKit::toAPI(selectedRect), WebKit::toAPI(selectedText.impl()), m_client.base.clientInfo);
        }

        void didChangeCompositionState(WebKit::PlayStationWebView& view, const WebCore::IntRect& compositionRect)
        {
            if (!m_client.didChangeCompositionState)
                return;
            m_client.didChangeCompositionState(WebKit::toAPI(&view), WebKit::toAPI(compositionRect), m_client.base.clientInfo);
        }

        void setCursorPosition(WebKit::PlayStationWebView& view, const WebCore::IntPoint& cursorPosition)
        {
            if (!m_client.setCursorPosition)
                return;
            m_client.setCursorPosition(WebKit::toAPI(&view), WebKit::toAPI(cursorPosition), m_client.base.clientInfo);
        }

        void setCursor(WebKit::PlayStationWebView& view, const WebCore::Cursor& cursor) final
        {
            if (!m_client.setCursor)
                return;
            m_client.setCursor(WebKit::toAPI(&view), toWKCursorType(cursor), m_client.base.clientInfo);
        }

        void doneWithMouseUpEvent(WebKit::PlayStationWebView& view, bool wasEventHandled)
        {
            if (!m_client.doneWithMouseUpEvent)
                return;
            m_client.doneWithMouseUpEvent(WebKit::toAPI(&view), wasEventHandled, m_client.base.clientInfo);
        }

        void doneWithKeyboardEvent(WebKit::PlayStationWebView& view, const WebKit::NativeWebKeyboardEvent& event, bool wasEventHandled)
        {
            if (!m_client.doneWithKeyboardEvent)
                return;
            m_client.doneWithKeyboardEvent(WebKit::toAPI(&view), toWKKeyboardEvent(event), wasEventHandled, m_client.base.clientInfo);
        }

        int launchOpenGLServerProcess(WebKit::PlayStationWebView& view)
        {
            if (!m_client.launchOpenGLServerProcess)
                return 0;
            return m_client.launchOpenGLServerProcess(WebKit::toAPI(&view), m_client.base.clientInfo);
        }

        void enterAcceleratedCompositingMode(WebKit::PlayStationWebView& view, const WebKit::LayerTreeContext& context)
        {
            if (!m_client.enterAcceleratedCompositingMode)
                return;
            m_client.enterAcceleratedCompositingMode(WebKit::toAPI(&view), (uint32_t)context.contextID, m_client.base.clientInfo);
        }

        void exitAcceleratedCompositingMode(WebKit::PlayStationWebView& view)
        {
            if (!m_client.exitAcceleratedCompositingMode)
                return;
            m_client.exitAcceleratedCompositingMode(WebKit::toAPI(&view), m_client.base.clientInfo);
        }

        void didChangeContentsVisibility(WebKit::PlayStationWebView& view, const WebCore::IntSize& size, const WebCore::FloatPoint& position, float scale)
        {
            if (!m_client.didChangeContentsVisibility)
                return;
            m_client.didChangeContentsVisibility(WebKit::toAPI(&view), WebKit::toAPI(size), WebKit::toAPI(roundedIntPoint(position)), scale, m_client.base.clientInfo);
        }
    };

    WebKit::toImpl(view)->setClient(makeUnique<ViewClient>(client));
}

void WKViewSetViewPopupMenuClient(WKViewRef view, const WKViewPopupMenuClientBase* client)
{
    class ViewPopupMenuClient final : public API::Client<WKViewPopupMenuClientBase>, public API::ViewPopupMenuClient {
    public:
        explicit ViewPopupMenuClient(const WKViewPopupMenuClientBase* client)
        {
            initialize(client);
        }

    private:
        void showPopupMenu(WebKit::PlayStationWebView& view, const Vector<WebKit::WebPopupItem>& items, const WebCore::IntRect& rect, int32_t selectedIndex)
        {
            if (!m_client.showPopupMenu) {
                view.valueChangedForPopupMenu(selectedIndex);
                return;
            }
            Vector<RefPtr<API::Object> > popupMenuItems;

            for (size_t i = 0; i < items.size(); i++)
                popupMenuItems.append(WebKit::WebPopupMenuItem::create(items[i]));

            RefPtr<API::Array> itemsArray;
            if (!popupMenuItems.isEmpty())
                itemsArray = API::Array::create(WTFMove(popupMenuItems));

            m_client.showPopupMenu(WebKit::toAPI(&view), WebKit::toAPI(itemsArray.get()), WebKit::toAPI(rect), selectedIndex, m_client.base.clientInfo);
        }

        void hidePopupMenu(WebKit::PlayStationWebView& view)
        {
            if (!m_client.hidePopupMenu)
                return;
            m_client.hidePopupMenu(WebKit::toAPI(&view), m_client.base.clientInfo);
        }
    };

    WebKit::toImpl(view)->setPopupMenuClient(makeUnique<ViewPopupMenuClient>(client));
}
