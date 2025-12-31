/*
 * Copyright (C) 2018 Sony Computer Entertainment Inc.
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
#include "WebEventFactory.h"

#include <WTF/WallTime.h>
#include <WebCore/Scrollbar.h>
#include <wpe/wpe.h>

using namespace WebCore;

namespace WebKit {

WebEventType webEventTypeForEvent(const WKKeyboardEvent& event)
{
    switch (event.type) {
    case kWKEventKeyDown: return WebEventType::KeyDown;
    case kWKEventKeyUp: return WebEventType::KeyUp;
    case kWKEventRawKeyDown: return WebEventType::RawKeyDown;
    case kWKEventChar: return WebEventType::Char;
    }
    ASSERT_NOT_REACHED();
    return WebEventType::KeyDown;
}

WebKeyboardEvent WebEventFactory::createWebKeyboardEvent(const WKKeyboardEvent& event)
{
    WebEventType type = webEventTypeForEvent(event);
    const String text = WTF::String::fromUTF8(event.text);
    const String unmodifiedText = WTF::String::fromUTF8(event.text);
    const auto keyIdentifier = WTF::String::fromLatin1(event.keyIdentifier);
    const String key = WTF::String::fromUTF8(event.key);
    const String code = WTF::String::fromUTF8(event.code);

    return WebKeyboardEvent({ type, toWebEventModifiers(event.modifiers), WTF::WallTime::now() },
        text,
        unmodifiedText,
        key,
        code,
        keyIdentifier,
        event.virtualKeyCode,
        event.virtualKeyCode,
        event.virtualKeyCode);
}

OptionSet<WebEventModifier> WebEventFactory::toWebEventModifiers(WKEventModifiers modifiers)
{
    OptionSet<WebEventModifier> modifierSet;

    if (modifiers & kWKEventModifiersControlKey)
        modifierSet.add(WebEventModifier::ControlKey);
    if (modifiers & kWKEventModifiersShiftKey)
        modifierSet.add(WebEventModifier::ShiftKey);
    if (modifiers & kWKEventModifiersAltKey)
        modifierSet.add(WebEventModifier::AltKey);
    if (modifiers & kWKEventModifiersMetaKey)
        modifierSet.add(WebEventModifier::MetaKey);
    if (modifiers & kWKEventModifiersCapsLockKey)
        modifierSet.add(WebEventModifier::CapsLockKey);

    return modifierSet;
}

// FIXME: The following are from libwpe's WebEventFactory. We'll switch to it after WebKeyboardEvent migration.

static OptionSet<WebEventModifier> modifiersForEventModifiers(unsigned eventModifiers)
{
    OptionSet<WebEventModifier> modifiers;
    if (eventModifiers & wpe_input_keyboard_modifier_control)
        modifiers.add(WebEventModifier::ControlKey);
    if (eventModifiers & wpe_input_keyboard_modifier_shift)
        modifiers.add(WebEventModifier::ShiftKey);
    if (eventModifiers & wpe_input_keyboard_modifier_alt)
        modifiers.add(WebEventModifier::AltKey);
    if (eventModifiers & wpe_input_keyboard_modifier_meta)
        modifiers.add(WebEventModifier::MetaKey);
    return modifiers;
}

WallTime wallTimeForEventTime(uint64_t msTimeStamp)
{
    // WPE event time field is an uint32_t, too small for full ms timestamps since
    // the epoch, and are expected to be just timestamps with monotonic behavior
    // to be compared among themselves, not against WallTime-like measurements.
    // Thus the need to define a reference origin based on the first event received.

    static uint64_t firstEventTimeStamp;
    static WallTime firstEventWallTime;
    static std::once_flag once;

    // Fallback for zero timestamps.
    if (!msTimeStamp)
        return WallTime::now();

    std::call_once(once, [msTimeStamp]() {
        firstEventTimeStamp = msTimeStamp;
        firstEventWallTime = WallTime::now();
    });

    uint64_t delta = msTimeStamp - firstEventTimeStamp;

    return firstEventWallTime + Seconds(delta / 1000.);
}

static inline short pressedMouseButtons(uint32_t modifiers)
{
    // MouseEvent.buttons
    // https://www.w3.org/TR/uievents/#ref-for-dom-mouseevent-buttons-1

    // 0 MUST indicate no button is currently active.
    short buttons = 0;

    // 1 MUST indicate the primary button of the device (in general, the left button or the only button on
    // single-button devices, used to activate a user interface control or select text).
    if (modifiers & wpe_input_pointer_modifier_button1)
        buttons |= 1;

    // 2 MUST indicate the secondary button (in general, the right button, often used to display a context menu),
    // if present.
    if (modifiers & wpe_input_pointer_modifier_button2)
        buttons |= 2;

    // 4 MUST indicate the auxiliary button (in general, the middle button, often combined with a mouse wheel).
    if (modifiers & wpe_input_pointer_modifier_button3)
        buttons |= 4;

    return buttons;
}

static inline int clickCount(WebEventType& type, WebMouseEventButton& button, const WebCore::IntPoint& position, const WTF::Seconds& timeStamp)
{
    static constexpr int dblclickTolerance = 4;
    static constexpr WTF::Seconds dblclickTime = WTF::Seconds::fromMilliseconds(500);
    static int lastClickCount;
    static WTF::Seconds lastClickTime;
    static WebMouseEventButton lastClickButton = WebMouseEventButton::NoButton;
    static WebCore::IntPoint lastClickPosition;
    const WebCore::IntPoint diffPosition(lastClickPosition - position);

    bool cancelPreviousClick = (std::abs(diffPosition.x()) > (dblclickTolerance / 2))
        || (std::abs(diffPosition.y()) > (dblclickTolerance / 2))
        || ((timeStamp - lastClickTime) > dblclickTime);

    if (type == WebEventType::MouseDown) {
        if (!cancelPreviousClick && (button == lastClickButton))
            ++lastClickCount;
        else {
            lastClickCount = 1;
            lastClickPosition = position;
        }
        lastClickTime = timeStamp;
        lastClickButton = button;
    } else if (type == WebEventType::MouseUp) {
        return 0;
    } else if (type == WebEventType::MouseMove) {
        if (cancelPreviousClick) {
            lastClickCount = 0;
            lastClickTime = WTF::Seconds(0);
            lastClickPosition = WebCore::IntPoint::zero();
        }
        return 0;
    }

    return lastClickCount;
}

WebMouseEvent WebEventFactory::createWebMouseEvent(struct wpe_input_pointer_event* event, float deviceScaleFactor, WebMouseEventSyntheticClickType)
{
    WebEventType type = WebEventType::NoType;
    switch (event->type) {
    case wpe_input_pointer_event_type_motion:
        type = WebEventType::MouseMove;
        break;
    case wpe_input_pointer_event_type_button:
        type = event->state ? WebEventType::MouseDown : WebEventType::MouseUp;
        break;
    case wpe_input_pointer_event_type_null:
        ASSERT_NOT_REACHED();
    }

    WebMouseEventButton button = WebMouseEventButton::NoButton;
    switch (event->type) {
    case wpe_input_pointer_event_type_motion:
    case wpe_input_pointer_event_type_button:
        if (event->button == 1)
            button = WebMouseEventButton::LeftButton;
        else if (event->button == 2)
            button = WebMouseEventButton::RightButton;
        else if (event->button == 3)
            button = WebMouseEventButton::MiddleButton;
        break;
    case wpe_input_pointer_event_type_null:
        ASSERT_NOT_REACHED();
    }

    // FIXME: Proper button support. deltaX/Y/Z. Click count.
    WebCore::IntPoint position(event->x, event->y);
    position.scale(1 / deviceScaleFactor);
    auto wallTime = wallTimeForEventTime(event->time);
    unsigned clickCount = WebKit::clickCount(type, button, position, wallTime.secondsSinceEpoch());
    return WebMouseEvent({ type, modifiersForEventModifiers(event->modifiers), wallTime }, button, pressedMouseButtons(event->modifiers), position, position,
        0, 0, 0, clickCount);
}

WebWheelEvent WebEventFactory::createWebWheelEvent(struct wpe_input_axis_event* event, float deviceScaleFactor, WebWheelEvent::Phase phase, WebWheelEvent::Phase momentumPhase)
{
    WebCore::IntPoint position(event->x, event->y);
    position.scale(1 / deviceScaleFactor);

    WebCore::FloatSize wheelTicks;
    WebCore::FloatSize delta;
    bool hasPreciseScrollingDeltas = false;

#if WPE_CHECK_VERSION(1, 5, 0)
    if (event->type & wpe_input_axis_event_type_mask_2d) {
        auto* event2D = reinterpret_cast<struct wpe_input_axis_2d_event*>(event);
        switch (event->type & (wpe_input_axis_event_type_mask_2d - 1)) {
        case wpe_input_axis_event_type_motion:
            wheelTicks = WebCore::FloatSize(std::copysign(1, event2D->x_axis), std::copysign(1, event2D->y_axis));
            delta = wheelTicks;
            delta.scale(WebCore::Scrollbar::pixelsPerLineStep());
            break;
        case wpe_input_axis_event_type_motion_smooth:
            wheelTicks = WebCore::FloatSize(event2D->x_axis / deviceScaleFactor, event2D->y_axis / deviceScaleFactor);
            delta = wheelTicks;
            hasPreciseScrollingDeltas = true;
            break;
        default:
            return WebWheelEvent();
        }

        return WebWheelEvent({ WebEventType::Wheel, OptionSet<WebEventModifier> { }, wallTimeForEventTime(event->time) }, position, position,
            delta, wheelTicks, WebWheelEvent::ScrollByPixelWheelEvent, phase, momentumPhase,
            hasPreciseScrollingDeltas);
    }
#endif

    // FIXME: We shouldn't hard-code this.
    enum Axis {
        Vertical,
        Horizontal,
        Smooth
    };

    switch (event->axis) {
    case Vertical:
        wheelTicks = WebCore::FloatSize(0, std::copysign(1, event->value));
        delta = wheelTicks;
        delta.scale(WebCore::Scrollbar::pixelsPerLineStep());
        break;
    case Horizontal:
        wheelTicks = WebCore::FloatSize(std::copysign(1, event->value), 0);
        delta = wheelTicks;
        delta.scale(WebCore::Scrollbar::pixelsPerLineStep());
        break;
    case Smooth:
        wheelTicks = WebCore::FloatSize(0, event->value / deviceScaleFactor);
        delta = wheelTicks;
        hasPreciseScrollingDeltas = true;
        break;
    default:
        return WebWheelEvent();
    };

    return WebWheelEvent({ WebEventType::Wheel, OptionSet<WebEventModifier> { }, wallTimeForEventTime(event->time) }, position, position,
        delta, wheelTicks, WebWheelEvent::ScrollByPixelWheelEvent, phase, momentumPhase,
        hasPreciseScrollingDeltas);
}

} // namespace WebKit
