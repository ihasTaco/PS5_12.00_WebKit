/*
 * (C) 2018 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(ACCESSIBILITY)
#include "APIObject.h"

#include <WebCore/AccessibilityObject.h>
#include <WebCore/FrameIdentifier.h>
#include <WebCore/LayoutRect.h>

namespace IPC {
class Decoder;
class Encoder;
}

namespace WebKit {

class WebAccessibilityObjectData {
public:
    explicit WebAccessibilityObjectData() { };
    WebAccessibilityObjectData(WebCore::AXCoreObject* axObject);
    ~WebAccessibilityObjectData();

    // IPC
    void encode(IPC::Encoder&) const;
    static WARN_UNUSED_RETURN bool decode(IPC::Decoder&, WebAccessibilityObjectData&);

    void set(WebCore::AXCoreObject* axObject);

    std::optional<WebCore::FrameIdentifier> webFrameID() const { return m_webFrameID; }
    WebCore::AccessibilityRole role() const { return m_role; };
    String title() const { return m_title; };
    String description() const { return m_description; };
    String helpText() const { return m_helpText; };
    URL url() const { return m_url; };
    WebCore::IntRect rect() const { return m_rect; };

    WebCore::AccessibilityButtonState buttonState() const { return m_buttonState; };
    String value() const { return m_value; };
    bool isFocused() const;
    bool isDisabled() const;
    bool isSelected() const;
    bool isVisited() const;
    bool isLinked() const;

private:
    String getAccessibilityTitle(WebCore::AXCoreObject* axObject, Vector<WebCore::AccessibilityText>& textOrder);
    String getAccessibilityDescription(WebCore::AXCoreObject* axObject, Vector<WebCore::AccessibilityText>& textOrder);
    String getAccessibilityHelpText(WebCore::AXCoreObject* axObject, Vector<WebCore::AccessibilityText>& textOrder);

    bool isImageLinked(WebCore::AXCoreObject*);
    bool isImageVisited(WebCore::AXCoreObject*);

    void setValue(WebCore::AXCoreObject*);
    void setState(WebCore::AXCoreObject*);

    // common
    std::optional<WebCore::FrameIdentifier> m_webFrameID;
    WebCore::AccessibilityRole m_role;
    String m_title;
    String m_description;
    String m_helpText;
    URL m_url;
    WebCore::IntRect m_rect;

    // checkbox or radio button state
    WebCore::AccessibilityButtonState m_buttonState;
    String m_value;

    enum State {
        StateNone = 0,
        StateFocused = 1 << 0,
        StateUnavailable = 1 << 1,
        StateSelected = 1 << 2,
        StateVisited = 1 << 3,
        StateLinked = 1 << 4,
    };
    typedef uint64_t States;
    States m_state;
};

} // namespace WebKit

#endif // ENABLE(ACCESSIBILITY)
