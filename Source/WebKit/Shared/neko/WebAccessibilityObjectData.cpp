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
#include "WebAccessibilityObjectData.h"

#if ENABLE(ACCESSIBILITY)

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "Encoder.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include <WebCore/FrameDestructionObserverInlines.h>
#include <WebCore/FocusDirection.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/HTMLAnchorElement.h>
#include <WebCore/LocalFrame.h>
#include <WebCore/LocalFrameLoaderClient.h>
#include <WebCore/RenderStyle.h>

using namespace WebCore;

namespace WebKit {

// WebAccessibilityObjectData
WebAccessibilityObjectData::WebAccessibilityObjectData(WebCore::AXCoreObject* axObject)
    : m_role(WebCore::AccessibilityRole::Unknown)
    , m_url(URL())
    , m_buttonState(AccessibilityButtonState::Off)
    , m_state(State::StateNone)
{
    set(axObject);
}

WebAccessibilityObjectData::~WebAccessibilityObjectData()
{
}

void WebAccessibilityObjectData::encode(IPC::Encoder& encoder) const
{
    encoder << m_webFrameID;
    encoder << static_cast<uint32_t>(m_role);
    encoder << m_title;
    encoder << m_description;
    encoder << m_helpText;
    encoder << m_url;
    encoder << m_rect;
    encoder << static_cast<uint32_t>(m_buttonState);
    encoder << m_value;
    encoder << m_state;
}

bool WebAccessibilityObjectData::decode(IPC::Decoder& decoder, WebAccessibilityObjectData& data)
{
    if (!decoder.decode(data.m_webFrameID))
        return false;
    uint32_t role;
    if (!decoder.decode(role))
        return false;
    data.m_role = static_cast<AccessibilityRole>(role);

    if (!decoder.decode(data.m_title))
        return false;
    if (!decoder.decode(data.m_description))
        return false;
    if (!decoder.decode(data.m_helpText))
        return false;
    if (!decoder.decode(data.m_url))
        return false;

    if (!decoder.decode(data.m_rect))
        return false;

    uint32_t buttonState;
    if (!decoder.decode(buttonState))
        return false;
    data.m_buttonState = static_cast<AccessibilityButtonState>(buttonState);
    
    if (!decoder.decode(data.m_value))
        return false;

    if (!decoder.decode(data.m_state))
        return false;
    return true;
}

bool WebAccessibilityObjectData::isFocused() const
{
    return m_state & State::StateFocused;
}

bool WebAccessibilityObjectData::isDisabled() const
{
    return m_state & State::StateUnavailable;
}

bool WebAccessibilityObjectData::isSelected() const
{
    return m_state & State::StateSelected;
}

bool WebAccessibilityObjectData::isVisited() const
{
    return m_state & State::StateVisited;
}

bool WebAccessibilityObjectData::isLinked() const
{
    return m_state & State::StateLinked;
}

String WebAccessibilityObjectData::getAccessibilityTitle(AXCoreObject* axObject, Vector<AccessibilityText>& textOrder)
{
    if (axObject->isListBoxOption())
        return axObject->stringValue();

    if (axObject->isPopUpButton())
        return axObject->stringValue();

    // For input fields, use only the text of the label element.
    if (axObject->isTextControl() && axObject->correspondingLabelForControlElement())
        return axObject->correspondingLabelForControlElement()->textUnderElement();

    for (const auto& text : textOrder) {
        if (text.textSource == AccessibilityTextSource::Alternative)
            break;

        if (text.textSource == AccessibilityTextSource::Visible || text.textSource == AccessibilityTextSource::Children)
            return text.text;

        // If there's an element that labels this object, then we should use
        // that text as our title.
        if (text.textSource == AccessibilityTextSource::LabelByElement)
            return text.text;

        // FIXME: The title tag is used in certain cases for the title. This usage should
        // probably be in the description field since it's not "visible".
        if (text.textSource == AccessibilityTextSource::TitleTag)
            return text.text;
    }

    return { };
}

String WebAccessibilityObjectData::getAccessibilityDescription(AXCoreObject* axObject, Vector<AccessibilityText>& textOrder)
{
    if (axObject->roleValue() == AccessibilityRole::StaticText)
        return axObject->textUnderElement();

    for (const auto& text : textOrder) {
        if (text.textSource == AccessibilityTextSource::Alternative)
            return text.text;

        if (text.textSource == AccessibilityTextSource::TitleTag)
            return text.text;
    }

    return { };
}

String WebAccessibilityObjectData::getAccessibilityHelpText(AXCoreObject*, Vector<AccessibilityText>& textOrder)
{
    bool descriptiveTextAvailable = false;
    for (const auto& text : textOrder) {
        if (text.textSource == AccessibilityTextSource::Help || text.textSource == AccessibilityTextSource::Summary)
            return text.text;

        switch (text.textSource) {
        case AccessibilityTextSource::Alternative:
        case AccessibilityTextSource::Visible:
        case AccessibilityTextSource::Children:
        case AccessibilityTextSource::LabelByElement:
            descriptiveTextAvailable = true;
        default:
            break;
        }

        if (text.textSource == AccessibilityTextSource::TitleTag && descriptiveTextAvailable)
            return text.text;
    }

    return { };
}

bool WebAccessibilityObjectData::isImageLinked(AXCoreObject* axObject)
{
    AccessibilityObject* obj = (AccessibilityObject*)axObject;

    Element* anchor = obj->anchorElement();
    if (!is<HTMLAnchorElement>(anchor))
        return false;

    if (downcast<HTMLAnchorElement>(*anchor).href().isEmpty())
        return false;

    return true;
}

bool WebAccessibilityObjectData::isImageVisited(AXCoreObject* axObject)
{
    AccessibilityObject* obj = (AccessibilityObject*)axObject;

    auto* style = obj->style();
    if (style && style->insideLink() == InsideLink::InsideVisited)
        return true;

    return false;
}

void WebAccessibilityObjectData::setValue(AXCoreObject* axObject)
{
    if (axObject->supportsRangeValue())
        m_value = String::number(axObject->valueForRange());
    else if (axObject->roleValue() == AccessibilityRole::SliderThumb)
        m_value = String::number(axObject->parentObject()->valueForRange());
    else if (axObject->isHeading())
        m_value = String::number(axObject->headingLevel());
    else if (axObject->isTextControl()) {
        if (!axObject->stringValue().isEmpty())
            m_value = axObject->stringValue(); // inputted text
        else
            m_value = axObject->placeholderValue();
    } else
        m_value = String();
}

void WebAccessibilityObjectData::setState(AXCoreObject* axObject)
{
    m_state = State::StateNone;

    if (axObject && axObject->element() && axObject->document()) {
        if (axObject->element() == axObject->document()->focusNavigationStartingNode(FocusDirection::None))
            m_state |= State::StateFocused;
    }

    if (!axObject->isEnabled())
        m_state |= State::StateUnavailable;
    else if (axObject->isTextControl() && !axObject->canSetValueAttribute()) // Also determine whether it is readonly
        m_state |= State::StateUnavailable;

    if (axObject->isSelected())
        m_state |= State::StateSelected;

    if (axObject->isImage()) {
        if (isImageLinked(axObject)) {
            m_state |= State::StateLinked;
            if (isImageVisited(axObject))
                m_state |= State::StateVisited;
        }
    }

    if (axObject->isLink()) {
        m_state |= State::StateLinked;
        if (axObject->isVisited())
            m_state |= State::StateVisited;
    }
}

void WebAccessibilityObjectData::set(AXCoreObject* axObject)
{
    if (!axObject)
        return;
    Document* document = axObject->document();
    if (!document || !document->frame())
        return;
    WebLocalFrameLoaderClient* webFrameLoaderClient = toWebLocalFrameLoaderClient(document->frame()->loader().client());
    if (!webFrameLoaderClient)
        return;

    m_webFrameID = webFrameLoaderClient->webFrame().frameID();

    if (axObject->isStaticText()) {
        if (Node* node = axObject->node()) {
            // If link and heading, prefer link.
            if (AXCoreObject* obj = AccessibilityObject::anchorElementForNode(node))
                axObject = obj;
            else {
                if (AXCoreObject* obj = AccessibilityObject::headingElementForNode(node))
                    axObject = obj;
            }
        }
    }

    m_role = axObject->roleValue();

    Vector<AccessibilityText> textOrder;
    axObject->accessibilityText(textOrder);
    m_title = getAccessibilityTitle(axObject, textOrder);
    m_description = getAccessibilityDescription(axObject, textOrder);
    m_helpText = getAccessibilityHelpText(axObject, textOrder);

    // link or frame url
    m_url = axObject->url();

    // checkbox or radio button state
    m_buttonState = axObject->checkboxOrRadioValue();

    setValue(axObject);
    setState(axObject);

    // object rectangle
    // We would like to use relativeFrame()/elementRect(), 
    // but there are cases where the rectangle size becomes incorrect when zooming.
    // Therefore, if there is a Node, use that.
    // If there is no Node, use relativeFrame().
    FloatRect rect;
    if (Node* node = axObject->node()) {
        bool isReplaced;
        rect = axObject->convertFrameToSpace(node->renderRect(&isReplaced), AccessibilityConversionSpace::Page);
    } else
        rect = axObject->relativeFrame();
    m_rect = WebCore::enclosingIntRect(rect);

    // ListBox, ListBoxOption
    if (axObject->isListBox()) {
        AccessibilityObject::AccessibilityChildrenVector children = axObject->selectedChildren();
        m_value = String::number(children.size());
    }
}

} // namespace WebKit

#endif // ENABLE(ACCESSIBILITY)
