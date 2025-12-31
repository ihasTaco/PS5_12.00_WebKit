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

#include "config.h"
#include "WebAccessibilityObject.h"

#if ENABLE(ACCESSIBILITY)

#include "WebFrameProxy.h"

using namespace WebCore;

namespace WebKit {

RefPtr<WebAccessibilityObject> WebAccessibilityObject::create(const WebAccessibilityObjectData& data, WebFrameProxy* frame)
{
    return adoptRef(new WebAccessibilityObject(data, frame));
}

WebAccessibilityObject::WebAccessibilityObject(const WebAccessibilityObjectData& data, WebFrameProxy* frame)
    : m_frame(frame)
    , m_data(data)
{
}

AccessibilityRole WebAccessibilityObject::role() const
{
    return m_data.role();
}

String WebAccessibilityObject::title() const
{
    return m_data.title();
}

String WebAccessibilityObject::description() const
{
    return m_data.description();
}

String WebAccessibilityObject::helpText() const
{
    return m_data.helpText();
}

URL WebAccessibilityObject::url() const
{
    return m_data.url();
}

IntRect WebAccessibilityObject::rect() const
{
    return m_data.rect();
}

AccessibilityButtonState WebAccessibilityObject::buttonState()
{
    return m_data.buttonState();
}

String WebAccessibilityObject::value() const
{
    return m_data.value();
}

bool WebAccessibilityObject::isFocused() const
{
    return m_data.isFocused();
}

bool WebAccessibilityObject::isDisabled() const
{
    return m_data.isDisabled();
}

bool WebAccessibilityObject::isSelected() const
{
    return m_data.isSelected();
}

bool WebAccessibilityObject::isVisited() const
{
    return m_data.isVisited();
}

bool WebAccessibilityObject::isLinked() const
{
    return m_data.isLinked();
}

WebPageProxy* WebAccessibilityObject::page()
{
    return frame()->page();
}

} // namespace WebKit

#endif // ENABLE(ACCESSIBILITY)
