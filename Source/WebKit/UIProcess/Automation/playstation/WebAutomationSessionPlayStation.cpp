/*
 * Copyright (C) 2019 Sony Interactive Entertainment Inc.
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
#include "WebAutomationSession.h"

#include "KeyboardCodes.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "WebEventFactory.h"
#include "WebPageProxy.h"
#include <WebKit/WKEventPlayStation.h>
#include <wpe/wpe.h>

namespace WebKit {
using namespace WebCore;

static uint32_t modifiersToEventState(OptionSet<WebEventModifier> modifiers)
{
    uint32_t state = 0;
    if (modifiers.contains(WebEventModifier::ControlKey))
        state |= wpe_input_keyboard_modifier_control;
    if (modifiers.contains(WebEventModifier::ShiftKey))
        state |= wpe_input_keyboard_modifier_shift;
    if (modifiers.contains(WebEventModifier::AltKey))
        state |= wpe_input_keyboard_modifier_alt;
    return state;
}

static unsigned mouseButtonToWPEButton(MouseButton button)
{
    switch (button) {
    case MouseButton::None:
    case MouseButton::Left:
        return 1;
    case MouseButton::Middle:
        return 3;
    case MouseButton::Right:
        return 2;
    }
    return 1;
}

static unsigned stateModifierForWPEButton(unsigned button)
{
    uint32_t state = 0;

    switch (button) {
    case 1:
        state |= wpe_input_pointer_modifier_button1;
        break;
    case 2:
        state |= wpe_input_pointer_modifier_button2;
        break;
    case 3:
        state |= wpe_input_pointer_modifier_button3;
        break;
    default:
        break;
    }

    return state;
}

static void doMouseEvent(WebPageProxy& page, const WebCore::IntPoint& location, unsigned button, unsigned state, uint32_t modifiers)
{
    const float deviceScaleFactor = 1;
    struct wpe_input_pointer_event event { wpe_input_pointer_event_type_button, 0, location.x(), location.y(), button, static_cast<uint32_t>(state ? 1 : 0), modifiers };
    page.handleMouseEvent(NativeWebMouseEvent(&event, deviceScaleFactor));
}

static void doMotionEvent(WebPageProxy& page, const WebCore::IntPoint& location, uint32_t modifiers)
{
    const float deviceScaleFactor = 1;
    struct wpe_input_pointer_event event { wpe_input_pointer_event_type_motion, 0, location.x(), location.y(), 0, 0, modifiers };
    page.handleMouseEvent(NativeWebMouseEvent(&event, deviceScaleFactor));
}

void WebAutomationSession::platformSimulateMouseInteraction(WebPageProxy& page, MouseInteraction interaction, MouseButton button, const WebCore::IntPoint& locationInView, OptionSet<WebEventModifier> keyModifiers, const String& pointerType)
{
    UNUSED_PARAM(pointerType);

    unsigned wpeButton = mouseButtonToWPEButton(button);
    auto modifier = stateModifierForWPEButton(wpeButton);
    uint32_t state = modifiersToEventState(keyModifiers) | m_currentModifiers;

    switch (interaction) {
    case MouseInteraction::Move:
        doMotionEvent(page, locationInView, state);
        break;
    case MouseInteraction::Down:
        m_currentModifiers |= modifier;
        doMouseEvent(page, locationInView, wpeButton, 1, state | modifier);
        break;
    case MouseInteraction::Up:
        m_currentModifiers &= ~modifier;
        doMouseEvent(page, locationInView, wpeButton, 0, state & ~modifier);
        break;
    case MouseInteraction::SingleClick:
        doMouseEvent(page, locationInView, wpeButton, 1, state | modifier);
        doMouseEvent(page, locationInView, wpeButton, 0, state);
        break;
    case MouseInteraction::DoubleClick:
        doMouseEvent(page, locationInView, wpeButton, 1, state | modifier);
        doMouseEvent(page, locationInView, wpeButton, 0, state);
        doMouseEvent(page, locationInView, wpeButton, 1, state | modifier);
        doMouseEvent(page, locationInView, wpeButton, 0, state);
        break;
    }
}

static int keyCodeForVirtualKey(Inspector::Protocol::Automation::VirtualKey key)
{
    switch (key) {
    case Inspector::Protocol::Automation::VirtualKey::Shift:
        return VK_LSHIFT;
    case Inspector::Protocol::Automation::VirtualKey::ShiftRight:
        return VK_RSHIFT;
    case Inspector::Protocol::Automation::VirtualKey::Control:
        return VK_LCONTROL;
    case Inspector::Protocol::Automation::VirtualKey::ControlRight:
        return VK_RCONTROL;
    case Inspector::Protocol::Automation::VirtualKey::Alternate:
        return VK_LMENU;
    case Inspector::Protocol::Automation::VirtualKey::AlternateRight:
        return VK_RMENU;
    case Inspector::Protocol::Automation::VirtualKey::Meta:
        return VK_LMENU;   /* TODO: is this valid? */
    case Inspector::Protocol::Automation::VirtualKey::MetaRight:
        return VK_RMENU;
    case Inspector::Protocol::Automation::VirtualKey::Command:
    case Inspector::Protocol::Automation::VirtualKey::CommandRight:
        return VK_EXECUTE;
    case Inspector::Protocol::Automation::VirtualKey::Help:
        return VK_HELP;
    case Inspector::Protocol::Automation::VirtualKey::Backspace:
        return VK_BACK;
    case Inspector::Protocol::Automation::VirtualKey::Tab:
        return VK_TAB;
    case Inspector::Protocol::Automation::VirtualKey::Clear:
        return VK_CLEAR;
    case Inspector::Protocol::Automation::VirtualKey::Enter:
        return VK_RETURN;
    case Inspector::Protocol::Automation::VirtualKey::Pause:
        return VK_PAUSE;
    case Inspector::Protocol::Automation::VirtualKey::Cancel:
        return VK_CANCEL;
    case Inspector::Protocol::Automation::VirtualKey::Escape:
        return VK_ESCAPE;
    case Inspector::Protocol::Automation::VirtualKey::PageUp:
    case Inspector::Protocol::Automation::VirtualKey::PageUpRight:
        return VK_PRIOR;
    case Inspector::Protocol::Automation::VirtualKey::PageDown:
    case Inspector::Protocol::Automation::VirtualKey::PageDownRight:
        return VK_NEXT;
    case Inspector::Protocol::Automation::VirtualKey::End:
    case Inspector::Protocol::Automation::VirtualKey::EndRight:
        return VK_END;
    case Inspector::Protocol::Automation::VirtualKey::Home:
    case Inspector::Protocol::Automation::VirtualKey::HomeRight:
        return VK_HOME;
    case Inspector::Protocol::Automation::VirtualKey::LeftArrow:
    case Inspector::Protocol::Automation::VirtualKey::LeftArrowRight:
        return VK_LEFT;
    case Inspector::Protocol::Automation::VirtualKey::UpArrow:
    case Inspector::Protocol::Automation::VirtualKey::UpArrowRight:
        return VK_UP;
    case Inspector::Protocol::Automation::VirtualKey::RightArrow:
    case Inspector::Protocol::Automation::VirtualKey::RightArrowRight:
        return VK_RIGHT;
    case Inspector::Protocol::Automation::VirtualKey::DownArrow:
    case Inspector::Protocol::Automation::VirtualKey::DownArrowRight:
        return VK_DOWN;
    case Inspector::Protocol::Automation::VirtualKey::Insert:
    case Inspector::Protocol::Automation::VirtualKey::InsertRight:
        return VK_INSERT;
    case Inspector::Protocol::Automation::VirtualKey::Delete:
    case Inspector::Protocol::Automation::VirtualKey::DeleteRight:
        return VK_DELETE;
    case Inspector::Protocol::Automation::VirtualKey::Space:
        return VK_SPACE;
    case Inspector::Protocol::Automation::VirtualKey::Semicolon:
        return VK_OEM_1;
    case Inspector::Protocol::Automation::VirtualKey::Equals:
        return VK_OEM_MINUS;
    case Inspector::Protocol::Automation::VirtualKey::Return:
        return VK_RETURN;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad0:
        return VK_NUMPAD0;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad1:
        return VK_NUMPAD1;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad2:
        return VK_NUMPAD2;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad3:
        return VK_NUMPAD3;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad4:
        return VK_NUMPAD4;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad5:
        return VK_NUMPAD5;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad6:
        return VK_NUMPAD6;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad7:
        return VK_NUMPAD7;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad8:
        return VK_NUMPAD8;
    case Inspector::Protocol::Automation::VirtualKey::NumberPad9:
        return VK_NUMPAD9;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadMultiply:
        return VK_MULTIPLY;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadAdd:
        return VK_ADD;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadSubtract:
        return VK_SUBTRACT;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadSeparator:
        return VK_SEPARATOR;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadDecimal:
        return VK_DECIMAL;
    case Inspector::Protocol::Automation::VirtualKey::NumberPadDivide:
        return VK_DIVIDE;
    case Inspector::Protocol::Automation::VirtualKey::Function1:
        return VK_F1;
    case Inspector::Protocol::Automation::VirtualKey::Function2:
        return VK_F2;
    case Inspector::Protocol::Automation::VirtualKey::Function3:
        return VK_F3;
    case Inspector::Protocol::Automation::VirtualKey::Function4:
        return VK_F4;
    case Inspector::Protocol::Automation::VirtualKey::Function5:
        return VK_F5;
    case Inspector::Protocol::Automation::VirtualKey::Function6:
        return VK_F6;
    case Inspector::Protocol::Automation::VirtualKey::Function7:
        return VK_F7;
    case Inspector::Protocol::Automation::VirtualKey::Function8:
        return VK_F8;
    case Inspector::Protocol::Automation::VirtualKey::Function9:
        return VK_F9;
    case Inspector::Protocol::Automation::VirtualKey::Function10:
        return VK_F10;
    case Inspector::Protocol::Automation::VirtualKey::Function11:
        return VK_F11;
    case Inspector::Protocol::Automation::VirtualKey::Function12:
        return VK_F12;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static int keyCodeForCharKey(CharKey charKey)
{
    if ('A' <= charKey && charKey <= 'Z')
        return VK_A + (charKey - 'A');
    if ('a' <= charKey && charKey <= 'z')
        return VK_A + (charKey - 'a');
    if ('0' <= charKey && charKey <= '9')
        return VK_0 + (charKey - '0');

    // TODO: other symbols (!"#$%&'()=~|`@[]+*;:<>,./\?_ etc...)
    // ASSERT_NOT_REACHED();
    return 0;
}

static unsigned modifiersForKeyCode(unsigned keyCode)
{
    switch (keyCode) {
    case VK_SHIFT: // just in case
    case VK_LSHIFT:
    case VK_RSHIFT:
        return kWKEventModifiersShiftKey;
    case VK_CONTROL: // just in case
    case VK_LCONTROL:
    case VK_RCONTROL:
        return kWKEventModifiersControlKey;
    case VK_MENU: // just in case
    case VK_LMENU:
    case VK_RMENU:
        return kWKEventModifiersAltKey;
    }
    return 0;
}

void WebAutomationSession::platformSimulateKeyboardInteraction(WebPageProxy& page, KeyboardInteraction interaction, std::variant<VirtualKey, CharKey>&& key)
{
    unsigned keyCode = 0;
    WTF::switchOn(key,
        [&](VirtualKey virtualKey) {
        keyCode = keyCodeForVirtualKey(virtualKey);
    },
        [&](CharKey charKey) {
        keyCode = keyCodeForCharKey(charKey);
    }
    );
    unsigned modifiers = modifiersForKeyCode(keyCode);

    WKEventType type;
    switch (interaction) {
    case KeyboardInteraction::KeyPress:
        m_currentModifiers |= modifiers;
        type = kWKEventKeyDown;
        break;
    case KeyboardInteraction::KeyRelease:
        m_currentModifiers &= ~modifiers;
        type = kWKEventKeyUp;
        break;
    case KeyboardInteraction::InsertByKey:
        type = kWKEventKeyDown;
        break;
    }

    auto event = WKKeyboardEventMake(
        type, kWKInputTypeNormal, nullptr, 0,
        "" /* TODO: keyIdentifier */,
        keyCode,
        "" /* TODO: key */,
        "" /* TODO: code */,
        -1, 0, m_currentModifiers
    );
    page.handleKeyboardEvent(event);
}

OptionSet<WebEventModifier> WebAutomationSession::platformWebModifiersFromRaw(unsigned modifiers)
{
    OptionSet<WebEventModifier> webModifiers;

    if (modifiers & kWKEventModifiersAltKey)
        webModifiers.add(WebEventModifier::AltKey);
    if (modifiers & kWKEventModifiersControlKey)
        webModifiers.add(WebEventModifier::ControlKey);
    if (modifiers & kWKEventModifiersShiftKey)
        webModifiers.add(WebEventModifier::ShiftKey);

    return webModifiers;
}

void WebAutomationSession::platformSimulateKeySequence(WebPageProxy& page, const String& keySequence)
{
    // https://www.w3.org/TR/uievents-code/#key-alphanumeric-writing-system
    String keyIdentifier = "Key" + keySequence.convertToASCIIUppercase();

    String key = (m_currentModifiers & kWKEventModifiersShiftKey)? keySequence.convertToASCIIUppercase(): keySequence;
    for (WKEventType type : {kWKEventKeyDown, kWKEventKeyUp}) {
        auto event = WKKeyboardEventMake(
            type, kWKInputTypeNormal, key.utf8().data(), 1,
            keyIdentifier.utf8().data(),
            0 /* TODO: virtualKeyCode */,
            "", /* TODO: key */
            "", /* TODO: code */
            -1, 0, m_currentModifiers
        );
        page.handleKeyboardEvent(event);
    }

}

} // namespace WebKit
