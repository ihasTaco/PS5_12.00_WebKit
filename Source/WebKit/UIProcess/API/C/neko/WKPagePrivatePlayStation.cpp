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
#include "WKPagePrivatePlayStation.h"

#include "DrawingAreaProxy.h"
#include "DrawingAreaProxyCoordinatedGraphics.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "WKAPICast.h"
#include "WebEventFactory.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/Region.h>
#include <cairo.h>
#include <wpe/wpe.h>

static void drawPageBackground(cairo_t* ctx, const std::optional<WebCore::Color>& backgroundColor, const WebCore::IntRect& rect)
{
    if (!backgroundColor || backgroundColor.value().isVisible())
        return;

    auto [r, g, b, a] = backgroundColor.value().toColorTypeLossy<WebCore::SRGBA<uint8_t>>().resolved();

    cairo_set_source_rgba(ctx, r, g, b, a);
    cairo_rectangle(ctx, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_set_operator(ctx, CAIRO_OPERATOR_OVER);
    cairo_fill(ctx);
}

static uint32_t wpeModifiersForWKEventModifiers(uint32_t eventModifiers)
{
    uint32_t modifiers = 0;
    if (eventModifiers & kWKEventModifiersControlKey)
        modifiers |= wpe_input_keyboard_modifier_control;
    if (eventModifiers & kWKEventModifiersShiftKey)
        modifiers |= wpe_input_keyboard_modifier_shift;
    if (eventModifiers & kWKEventModifiersAltKey)
        modifiers |= wpe_input_keyboard_modifier_alt;
    if (eventModifiers & kWKEventModifiersMetaKey)
        modifiers |= wpe_input_keyboard_modifier_meta;
    if (eventModifiers & kWKMouseEventModifiersLeftButton)
        modifiers |= wpe_input_pointer_modifier_button1;
    if (eventModifiers & kWKMouseEventModifiersRightButton)
        modifiers |= wpe_input_pointer_modifier_button2;
    if (eventModifiers & kWKMouseEventModifiersMiddleButton)
        modifiers |= wpe_input_pointer_modifier_button3;
    return modifiers;
}

#if 0 // Need switch NativeWebKeyboardEvent to the upstream variant
void WKPageHandleKeyboardEvent(WKPageRef pageRef, WKKeyboardEvent event)
{
    using WebKit::NativeWebKeyboardEvent;

    wpe_input_keyboard_event wpeEvent;
    wpeEvent.time = 0;
    wpeEvent.key_code = event.virtualKeyCode;
    wpeEvent.hardware_key_code = event.virtualKeyCode;
    wpeEvent.modifiers = 0; // TODO: Handle modifiers.
    switch (event.type) {
    case kWKEventKeyDown:
        wpeEvent.pressed = true;
        break;
    case kWKEventKeyUp:
        wpeEvent.pressed = false;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    NativeWebKeyboardEvent::HandledByInputMethod handledByInputMethod = NativeWebKeyboardEvent::HandledByInputMethod::No;
    std::optional<Vector<WebCore::CompositionUnderline>> preeditUnderlines;
    std::optional<WebKit::EditingRange> preeditSelectionRange;
    WebKit::toImpl(pageRef)->handleKeyboardEvent(NativeWebKeyboardEvent(&wpeEvent, "", handledByInputMethod, WTFMove(preeditUnderlines), WTFMove(preeditSelectionRange)));
}
#else
void WKPageHandleKeyboardEvent(WKPageRef pageRef, WKKeyboardEvent event)
{
    WebKit::NativeWebKeyboardEvent nativeEvent(event);

    if ((event.type == kWKEventKeyDown) && ((nativeEvent.keyIdentifier() == "Accept"_s) || (nativeEvent.keyIdentifier() == "Convert"_s) || (nativeEvent.keyIdentifier() == "Cancel"_s)))
        WebKit::toImpl(pageRef)->handleKeyboardEventPlayStation(nativeEvent, event.caretOffset);
    else if ((event.type == kWKEventRawKeyDown) && (nativeEvent.keyIdentifier() == "TabJump"_s))
        WebKit::toImpl(pageRef)->handleKeyboardEventPlayStation(nativeEvent, event.caretOffset);
    else
        WebKit::toImpl(pageRef)->handleKeyboardEvent(nativeEvent);
}
#endif

void WKPageHandleMouseEvent(WKPageRef pageRef, WKMouseEvent event)
{
    using WebKit::NativeWebMouseEvent;

    wpe_input_pointer_event wpeEvent;

    switch (event.type) {
    case kWKEventMouseDown:
        wpeEvent.type = wpe_input_pointer_event_type_button;
        wpeEvent.state = 1;
        break;
    case kWKEventMouseUp:
        wpeEvent.type = wpe_input_pointer_event_type_button;
        wpeEvent.state = 0;
        break;
    case kWKEventMouseMove:
        wpeEvent.type = wpe_input_pointer_event_type_motion;
        wpeEvent.state = 0;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    switch (event.button) {
    case kWKEventMouseButtonLeftButton:
        wpeEvent.button = 1;
        break;
    case kWKEventMouseButtonMiddleButton:
        wpeEvent.button = 3;
        break;
    case kWKEventMouseButtonRightButton:
        wpeEvent.button = 2;
        break;
    case kWKEventMouseButtonNoButton:
        wpeEvent.button = 0;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }
    wpeEvent.time = 0;
    wpeEvent.x = event.position.x;
    wpeEvent.y = event.position.y;
    wpeEvent.modifiers = wpeModifiersForWKEventModifiers(event.modifiers);

    const float deviceScaleFactor = 1;

    WebKit::toImpl(pageRef)->handleMouseEvent(NativeWebMouseEvent(&wpeEvent, deviceScaleFactor));
}

void WKPageHandleWheelEvent(WKPageRef pageRef, WKWheelEvent event)
{
    using WebKit::WebWheelEvent;
    using WebKit::NativeWebWheelEvent;

    wpe_input_axis_2d_event wpeEvent;
    wpeEvent.base.type = (wpe_input_axis_event_type)(wpe_input_axis_event_type_motion_smooth | wpe_input_axis_event_type_mask_2d);
    wpeEvent.base.time = 0;
    wpeEvent.base.x = event.position.x;
    wpeEvent.base.y = event.position.y;
    wpeEvent.base.axis = 0;
    wpeEvent.base.value = 0;
    wpeEvent.base.modifiers = wpeModifiersForWKEventModifiers(event.modifiers);
    wpeEvent.x_axis = event.delta.width;
    wpeEvent.y_axis = event.delta.height;

    const float deviceScaleFactor = 1;

    WebKit::toImpl(pageRef)->handleNativeWheelEvent(NativeWebWheelEvent(&wpeEvent.base, deviceScaleFactor, WebWheelEvent::Phase::PhaseNone, WebWheelEvent::Phase::PhaseNone));
}

void WKPageHandleImeEvent(WKPageRef pageRef, const WKImeEvent* event)
{
    WTF::String compositionString = WTF::String::fromUTF8(event->compositionString);

    if (event->imeEventType == UpdatePreedit) {
        Vector<WebCore::CompositionUnderline> underlines;
        underlines.append(WebCore::CompositionUnderline(0, compositionString.length(), WebCore::CompositionUnderlineColor::TextColor, WebCore::Color::black, false));
        WebKit::toImpl(pageRef)->setComposition(compositionString, WTFMove(underlines), event->caretIndex, event->caretIndex, 0, 0);
        return;
    }

    if ((event->imeEventType == ConfirmPreedit)
        || (event->imeEventType == InsertText)) {
        WebKit::toImpl(pageRef)->confirmComposition(compositionString, event->caretIndex, 0);
        return;
    }

    if (event->imeEventType == SetValueToFocusedNode)
        WebKit::toImpl(pageRef)->setValueToFocusedNode(compositionString);
}

void WKPagePaint(WKPageRef pageRef, unsigned char* surfaceData, WKSize wkSurfaceSize, WKRect wkPaintRect)
{
    auto surfaceSize = WebKit::toIntSize(wkSurfaceSize);
    auto paintRect = WebKit::toIntRect(wkPaintRect);
    if (!surfaceData || surfaceSize.isEmpty())
        return;

    const cairo_format_t format = CAIRO_FORMAT_ARGB32;
    cairo_surface_t* surface = cairo_image_surface_create_for_data(surfaceData, format, surfaceSize.width(), surfaceSize.height(), cairo_format_stride_for_width(format, surfaceSize.width()));
    if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
        return;

    auto page = WebKit::toImpl(pageRef);
    auto scale = page->deviceScaleFactor();
    cairo_surface_set_device_scale(surface, scale, scale);

    cairo_t* ctx = cairo_create(surface);

    auto& backgroundColor = page->backgroundColor();
    page->endPrinting();
    if (auto* drawingArea = static_cast<WebKit::DrawingAreaProxyCoordinatedGraphics*>(page->drawingArea())) {
        // FIXME: We should port WebKit1's rect coalescing logic here.
        WebCore::Region unpaintedRegion;
        drawingArea->paint(ctx, paintRect, unpaintedRegion);

        for (const auto& rect : unpaintedRegion.rects())
            drawPageBackground(ctx, backgroundColor, rect);
    } else
        drawPageBackground(ctx, backgroundColor, paintRect);

    cairo_destroy(ctx);
    cairo_surface_destroy(surface);
}

void WKPageBlurFocusedNode(WKPageRef pageRef)
{
    return WebKit::toImpl(pageRef)->exitComposition();
}

void WKPageSetCaretVisible(WKPageRef pageRef, bool visible)
{
    WebKit::toImpl(pageRef)->setCaretVisible(visible);
}

void WKPageSuspendActiveDOMObjectsAndAnimations(WKPageRef pageRef)
{
    WebKit::toImpl(pageRef)->suspendActiveDOMObjectsAndAnimations();
}

void WKPageResumeActiveDOMObjectsAndAnimations(WKPageRef pageRef)
{
    WebKit::toImpl(pageRef)->resumeActiveDOMObjectsAndAnimations();
}

void WKPageClearSelection(WKPageRef pageRef)
{
    WebKit::toImpl(pageRef)->clearSelectionWithoutBlur();
}

bool WKPageWebProcessIsResponsive(WKPageRef pageRef)
{
    return WebKit::toImpl(pageRef)->process().isResponsive();
}

void WKPageSetDrawsBackground(WKPageRef pageRef, bool drawsBackground)
{
    if (drawsBackground)
        WebKit::toImpl(pageRef)->setBackgroundColor(WebCore::Color(WebCore::Color::white));
    else
        WebKit::toImpl(pageRef)->setBackgroundColor(WebCore::Color(WebCore::Color::transparentBlack));
}

void WKPageScrollBy(WKPageRef pageRef, WKPoint scrollDirection)
{
    if (scrollDirection.x)
        WebKit::toImpl(pageRef)->scrollBy(scrollDirection.x > 0 ? WebCore::ScrollRight : WebCore::ScrollLeft, WebCore::ScrollGranularity::Line);
    if (scrollDirection.y)
        WebKit::toImpl(pageRef)->scrollBy(scrollDirection.y > 0 ? WebCore::ScrollDown : WebCore::ScrollUp, WebCore::ScrollGranularity::Line);
}

void WKPageSetPageScaleFactor(WKPageRef pageRef, double newScale)
{
    WebKit::toImpl(pageRef)->scalePageInViewCoordinates(newScale, WebCore::IntPoint());
}

double WKPageGetPageScaleFactor(WKPageRef pageRef)
{
    return  WebKit::toImpl(pageRef)->hasRunningProcess() ? WebKit::toImpl(pageRef)->pageScaleFactor() : 1.0;
}

void WKPageSetViewScaleFactor(WKPageRef pageRef, double newScale)
{
    WebKit::toImpl(pageRef)->scaleView(newScale);
}

double WKPageGetViewScaleFactor(WKPageRef pageRef)
{
    return WebKit::toImpl(pageRef)->hasRunningProcess() ? WebKit::toImpl(pageRef)->viewScaleFactor() : 1.0;
}
