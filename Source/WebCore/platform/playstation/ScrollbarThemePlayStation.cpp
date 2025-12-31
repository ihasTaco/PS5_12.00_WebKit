/*
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
#include "ScrollbarThemePlayStation.h"

#include "FloatRoundedRect.h"
#include "HostWindow.h"
#include "NotImplemented.h"
#include "ScrollView.h"
#include "Scrollbar.h"

namespace WebCore {

const int ScrollbarOffset = 12;

ScrollbarTheme& ScrollbarTheme::nativeTheme()
{
    static ScrollbarThemePlayStation theme;
    return theme;
}

int ScrollbarThemePlayStation::scrollbarThickness(ScrollbarWidth scrollbarWidth, ScrollbarExpansionState)
{   
    if (scrollbarWidth == ScrollbarWidth::None)
        return 0;
    return 9;
}

bool ScrollbarThemePlayStation::hasButtons(Scrollbar&)
{
    notImplemented();
    return true;
}

bool ScrollbarThemePlayStation::hasThumb(Scrollbar&)
{
    notImplemented();
    return true;
}

IntRect ScrollbarThemePlayStation::backButtonRect(Scrollbar&, ScrollbarPart, bool)
{
    notImplemented();
    return { };
}

IntRect ScrollbarThemePlayStation::forwardButtonRect(Scrollbar&, ScrollbarPart, bool)
{
    notImplemented();
    return { };
}

bool ScrollbarThemePlayStation::usesOverlayScrollbars() const
{
    return true;
}

IntRect ScrollbarThemePlayStation::trackRect(Scrollbar& scrollbar, bool)
{
    return scrollbar.frameRect();
}

void ScrollbarThemePlayStation::paintTrackBackground(GraphicsContext& context, Scrollbar&, const IntRect& trackRect)
{
    context.fillRect(trackRect, Color::transparentBlack);
}

void ScrollbarThemePlayStation::paintThumb(GraphicsContext& context, Scrollbar& scrollbar, const IntRect& thumbRect)
{
    if (scrollbar.enabled()) {
        IntRect rect = thumbRect;

        float radius = 0;
        int slimPixelSize = -1;

        // Slim and round thumb scrollbar
        if (scrollbar.orientation() == ScrollbarOrientation::Horizontal) {
            rect.inflateY(slimPixelSize);
            radius = FloatRect(rect).height() / 2;
        } else {
            rect.inflateX(slimPixelSize);
            radius = FloatRect(rect).width() / 2;
        }

        // paint first to make the rounded border
        // border: #FFFFFF opacity 24% 1px
        FloatRoundedRect roundedRectBorder(rect, FloatRoundedRect::Radii(radius));
        Color borderColor(SRGBA<uint8_t> { 0xFF, 0xFF, 0xFF });
        borderColor = borderColor.colorWithAlphaMultipliedBy(0.24);
        context.fillRoundedRect(roundedRectBorder, borderColor);

        // scrollbar: #292929 opacity 70%
        int borderPixelSize = 1;
        rect.inflateX(-borderPixelSize);
        rect.inflateY(-borderPixelSize);
        radius -= borderPixelSize;
        FloatRoundedRect roundedRect(rect, FloatRoundedRect::Radii(radius));
        Color thumbColor(SRGBA<uint8_t> { 0x29, 0x29, 0x29 });
        thumbColor = thumbColor.colorWithAlphaMultipliedBy(0.7);
        context.fillRoundedRect(roundedRect, thumbColor);
    }
}

IntRect ScrollbarThemePlayStation::constrainTrackRectToTrackPieces(Scrollbar& scrollbar, const IntRect& rect)
{
    IntRect offsetRect = rect;

    if (scrollbar.orientation() == ScrollbarOrientation::Horizontal)
        offsetRect.inflateX(-ScrollbarOffset); // left and right
    else
        offsetRect.shiftMaxYEdgeTo(rect.maxY() - ScrollbarOffset); // bottom only

    return offsetRect;
}

} // namespace WebCore
