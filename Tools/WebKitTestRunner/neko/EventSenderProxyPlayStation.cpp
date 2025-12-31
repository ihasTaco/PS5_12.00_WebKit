/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventSenderProxy.h"

#include "KeyboardEvents.h"
#include "PlatformWebView.h"
#include "TestController.h"
#include <WebKit/WKPagePrivatePlayStation.h>
#include <unistd.h>

namespace WTR {

// Key event location code defined in DOM Level 3.
enum KeyLocationCode {
    DOMKeyLocationStandard = 0x00,
    DOMKeyLocationLeft = 0x01,
    DOMKeyLocationRight = 0x02,
    DOMKeyLocationNumpad = 0x03
};

enum WTREventType {
    WTREventTypeNone = 0,
    WTREventTypeMouseDown,
    WTREventTypeMouseUp,
    WTREventTypeMouseMove,
    WTREventTypeMouseScrollBy,
    WTREventTypeLeapForward
};

struct WTREvent {
    WTREventType eventType;
    unsigned delay;
    WKEventModifiers modifiers;
    int horizontal;
    int vertical;

    WTREvent()
        : eventType(WTREventTypeNone)
        , delay(0)
        , modifiers(0)
        , horizontal(-1)
        , vertical(-1)
    {
    }

    WTREvent(WTREventType eventType, unsigned delay, WKEventModifiers modifiers)
        : eventType(eventType)
        , delay(delay)
        , modifiers(modifiers)
        , horizontal(-1)
        , vertical(-1)
    {
    }
};

struct KeyEventInfo {
    int key;
    const char* text;

    KeyEventInfo(int key, const char* text)
        : key(key)
        , text(text)
    {
    }
};

EventSenderProxy::EventSenderProxy(TestController* testController)
    : m_testController(testController)
    , m_time(0)
    , m_leftMouseButtonDown(false)
    , m_clickCount(0)
    , m_clickTime(0)
    , m_clickButton(kWKEventMouseButtonNoButton)
    , m_mouseButton(kWKEventMouseButtonNoButton)
#if ENABLE(TOUCH_EVENTS)
    , m_touchPoints(0)
#endif
{
}

EventSenderProxy::~EventSenderProxy()
{
#if ENABLE(TOUCH_EVENTS)
    clearTouchPoints();
#endif
}

void EventSenderProxy::updateClickCountForButton(int button)
{
    if (m_time - m_clickTime < 1 && m_position == m_clickPosition && button == m_clickButton) {
        ++m_clickCount;
        m_clickTime = m_time;
        return;
    }

    m_clickCount = 1;
    m_clickTime = m_time;
    m_clickPosition = m_position;
    m_clickButton = button;
}

KeyEventInfo getKeyInfoForKeyRef(WKStringRef keyRef, unsigned location, uint32_t* modifiers)
{
    if (location == DOMKeyLocationNumpad) {
        if (WKStringIsEqualToUTF8CString(keyRef, "leftArrow"))
            return KeyEventInfo(VK_NUMPAD4, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "rightArrow"))
            return KeyEventInfo(VK_NUMPAD6, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "upArrow"))
            return KeyEventInfo(VK_NUMPAD8, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "downArrow"))
            return KeyEventInfo(VK_NUMPAD2, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "pageUp"))
            return KeyEventInfo(VK_NUMPAD9, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "pageDown"))
            return KeyEventInfo(VK_NUMPAD3, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "home"))
            return KeyEventInfo(VK_NUMPAD7, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "end"))
            return KeyEventInfo(VK_NUMPAD1, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "insert"))
            return KeyEventInfo(VK_NUMPAD0, "");
        if (WKStringIsEqualToUTF8CString(keyRef, "delete"))
            return KeyEventInfo(VK_DECIMAL, "");

        return KeyEventInfo(0, "");
    }

    if (WKStringIsEqualToUTF8CString(keyRef, "leftControl"))
        return KeyEventInfo(VK_LCONTROL, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "rightControl"))
        return KeyEventInfo(VK_RCONTROL, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "leftShift"))
        return KeyEventInfo(VK_LSHIFT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "rightShift"))
        return KeyEventInfo(VK_RSHIFT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "leftAlt"))
        return KeyEventInfo(VK_LMENU, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "rightAlt"))
        return KeyEventInfo(VK_RMENU, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "leftArrow"))
        return KeyEventInfo(VK_LEFT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "rightArrow"))
        return KeyEventInfo(VK_RIGHT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "upArrow"))
        return KeyEventInfo(VK_UP, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "downArrow"))
        return KeyEventInfo(VK_DOWN, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "pageUp"))
        return KeyEventInfo(VK_PRIOR, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "pageDown"))
        return KeyEventInfo(VK_NEXT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "home"))
        return KeyEventInfo(VK_HOME, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "end"))
        return KeyEventInfo(VK_END, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "insert"))
        return KeyEventInfo(VK_INSERT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "delete"))
        return KeyEventInfo(VK_DELETE, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "printScreen"))
        return KeyEventInfo(VK_PRINT, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "menu"))
        return KeyEventInfo(VK_MENU, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F1"))
        return KeyEventInfo(VK_F1, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F2"))
        return KeyEventInfo(VK_F2, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F3"))
        return KeyEventInfo(VK_F3, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F4"))
        return KeyEventInfo(VK_F4, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F5"))
        return KeyEventInfo(VK_F5, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F6"))
        return KeyEventInfo(VK_F6, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F7"))
        return KeyEventInfo(VK_F7, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F8"))
        return KeyEventInfo(VK_F8, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F9"))
        return KeyEventInfo(VK_F9, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F10"))
        return KeyEventInfo(VK_F10, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F11"))
        return KeyEventInfo(VK_F11, "");
    if (WKStringIsEqualToUTF8CString(keyRef, "F12"))
        return KeyEventInfo(VK_F12, "");

    size_t bufferSize = WKStringGetMaximumUTF8CStringSize(keyRef);
    auto buffer = std::make_unique<char[]>(bufferSize);
    WKStringGetUTF8CString(keyRef, buffer.get(), bufferSize);
    char charCode = buffer.get()[0];

    if (charCode == '\n' || charCode == '\r')
        return KeyEventInfo(VK_RETURN, "\r");
    if (charCode == '\t')
        return KeyEventInfo(VK_TAB, "\t");
    if (charCode == '\x8')
        return KeyEventInfo(VK_BACK, "\x8");
    if (charCode == 0x001B)
        return KeyEventInfo(VK_ESCAPE, "\e");
    if (charCode == ' ')
        return KeyEventInfo(VK_SPACE, " ");

    auto text = makeString(buffer.get(), "\0"_s);
    if (isASCIIUpper(charCode)) {
        *modifiers |= kWKEventModifiersShiftKey;
        return KeyEventInfo(VK_A + charCode - 'A', text.utf8().data());
    }
    if (isASCIILower(charCode))
        return KeyEventInfo(VK_A + charCode - 'a', text.utf8().data());
    if (isASCIIDigit(charCode))
        return KeyEventInfo(VK_0 + charCode - '0', text.utf8().data());

    // Not sure if this is correct.
    return KeyEventInfo(charCode, "");
}

void EventSenderProxy::dispatchEvent(const WTREvent&)
{
    // Needs to actually happen
}

void EventSenderProxy::sendOrQueueEvent(const WTREvent& event)
{
    if (m_eventQueue.isEmpty() || !m_eventQueue.last().delay) {
        dispatchEvent(event);
        return;
    }

    m_eventQueue.append(event);
}

void EventSenderProxy::rawKeyDown(WKStringRef key, WKEventModifiers modifiers, unsigned keyLocation)
{
}

void EventSenderProxy::rawKeyUp(WKStringRef key, WKEventModifiers modifiers, unsigned keyLocation)
{
}

void EventSenderProxy::mouseDown(unsigned button, WKEventModifiers wkModifiers, WKStringRef pointerType)
{
    UNUSED_PARAM(pointerType);

    if (m_mouseButton == static_cast<int32_t>(button))
        return;

    WKViewRef view = m_testController->mainWebView()->platformView();
    WKPageHandleMouseEvent(WKViewGetPage(view), WKMouseEventMake(kWKEventMouseDown, kWKEventMouseButtonLeftButton, WKPointMake(m_position.x, m_position.y), 0));

    m_mouseButton = button;
    updateClickCountForButton(button);
}

void EventSenderProxy::mouseUp(unsigned button, WKEventModifiers wkModifiers, WKStringRef pointerType)
{
    UNUSED_PARAM(pointerType);

    if (m_mouseButton == static_cast<int32_t>(button))
        m_mouseButton = kWKEventMouseButtonNoButton;

    WKViewRef view = m_testController->mainWebView()->platformView();
    WKPageHandleMouseEvent(WKViewGetPage(view), WKMouseEventMake(kWKEventMouseUp, kWKEventMouseButtonLeftButton, WKPointMake(m_position.x, m_position.y), 0));

    m_clickPosition = m_position;
    m_clickTime = currentEventTime();
}

void EventSenderProxy::mouseMoveTo(double x, double y, WKStringRef pointerType)
{
    UNUSED_PARAM(pointerType);

    WKViewRef view = m_testController->mainWebView()->platformView();
    WKPageHandleMouseEvent(WKViewGetPage(view), WKMouseEventMake(kWKEventMouseMove, kWKEventMouseButtonNoButton, WKPointMake(x, y), 0));

    m_position.x = x;
    m_position.y = y;
}

void EventSenderProxy::mouseScrollBy(int horizontal, int vertical)
{
    WTREvent event(WTREventTypeMouseScrollBy, 0, 0);
    // We need to invert scrolling values since in EFL negative z value means that
    // canvas is scrolling down
    event.horizontal = -horizontal;
    event.vertical = -vertical;
}

void EventSenderProxy::continuousMouseScrollBy(int horizontal, int vertical, bool paged)
{
}

void EventSenderProxy::mouseScrollByWithWheelAndMomentumPhases(int x, int y, int /*phase*/, int /*momentum*/)
{
    // EFL does not have the concept of wheel gesture phases or momentum. Just relay to
    // the mouse wheel handler.
    mouseScrollBy(x, y);
}

void EventSenderProxy::leapForward(int milliseconds)
{
    if (m_eventQueue.isEmpty())
        m_eventQueue.append(WTREvent(WTREventTypeLeapForward, milliseconds, 0));

    m_time += milliseconds / 1000.0;
}

void EventSenderProxy::keyDown(WKStringRef keyRef, WKEventModifiers wkModifiers, unsigned location)
{
    uint32_t modifiers = wkModifiers;
    KeyEventInfo keyInfo = getKeyInfoForKeyRef(keyRef, location, &modifiers);
    WKViewRef view = m_testController->mainWebView()->platformView();
    WKKeyboardEvent event = WKKeyboardEventMake(kWKEventKeyDown, kWKInputTypeNormal, keyInfo.text, 1, keyIdentifierForKeyCode(keyInfo.key), keyInfo.key, "", "", -1, 0, modifiers);
    WKPageHandleKeyboardEvent(WKViewGetPage(view), event);
    event.type = kWKEventKeyUp;
    WKPageHandleKeyboardEvent(WKViewGetPage(view), event);
}

}
