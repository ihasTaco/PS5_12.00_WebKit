/*
 * Copyright (C) 2018 Sony Interactive Entertainment Inc.
 * Copyright (C) 2022 Apple Inc. All rights reserved.
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
#include "RenderThemePlayStation.h"

#include "BitmapImage.h"
#include "Color.h"
#include "FloatRoundedRect.h"
#include "Gradient.h"
#include "HTMLInputElement.h"
#include "HTMLMeterElement.h"
#include "RenderBox.h"
#include "RenderMeter.h"
#include "RenderProgress.h"
#include "RenderStyleSetters.h"
#include "SharedBuffer.h"
#include "UserAgentScripts.h"
#include "UserAgentStyleSheets.h"
#include <WebKitResources/WebKitResources.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

RenderTheme& RenderTheme::singleton()
{
    static NeverDestroyed<RenderThemePlayStation> theme;
    return theme;
}

RenderThemePlayStation::RenderThemePlayStation()
    : RenderTheme()
{

}

RenderThemePlayStation::~RenderThemePlayStation() = default;

Ref<Image> loadThemeImage(const char* name)
{
    auto img = BitmapImage::create();
    if (name) {
        const char* data;
        uint32_t size;
        WebKitResources::load(name, &data, &size);
        if (data && size > 0) {
            RefPtr<SharedBuffer> buffer = SharedBuffer::create(data, size);
            img->setData(WTFMove(buffer), true);
        }
    }
    return img;
}

void RenderThemePlayStation::paintThemeImage(GraphicsContext& ctx, Image& image, const FloatRect& rect)
{
    ctx.drawImage(image, rect);
}

void RenderThemePlayStation::paintThemeImage3x3(GraphicsContext& ctx, Image& image, const FloatRect& rect, int left, int right, int top, int bottom)
{
    FloatRect dstRect = rect;
    FloatRect srcRect = image.rect();

    // top-left
    ctx.drawImage(image, FloatRect(dstRect.x(), dstRect.y(), left, top), FloatRect(srcRect.x(), srcRect.y(), left, top));
    // top-middle
    ctx.drawImage(image, FloatRect(dstRect.x() + left, dstRect.y(), dstRect.width() - left - right, top), FloatRect(srcRect.x() + left, srcRect.y(), srcRect.width() - left - right, top));
    // top-right
    ctx.drawImage(image, FloatRect(dstRect.x() + dstRect.width() - right, dstRect.y(), right, top), FloatRect(srcRect.x() + srcRect.width() - right, srcRect.y(), right, top));
    // middle-left
    ctx.drawImage(image, FloatRect(dstRect.x(), dstRect.y() + top, left, dstRect.height() - top - bottom), FloatRect(srcRect.x(), srcRect.y() + top, left, srcRect.height() - top - bottom));
    // middle-middle
    ctx.drawImage(image, FloatRect(dstRect.x() + left, dstRect.y() + top, dstRect.width() - left - right, dstRect.height() - top - bottom), FloatRect(srcRect.x() + left, srcRect.y() + top, srcRect.width() - left - right, srcRect.height() - top - bottom));
    // middle-right
    ctx.drawImage(image, FloatRect(dstRect.x() + dstRect.width() - right, dstRect.y() + top, right, dstRect.height() - top - bottom), FloatRect(srcRect.x() + srcRect.width() - right, srcRect.y() + top, right, srcRect.height() - top - bottom));
    // bottom-left
    ctx.drawImage(image, FloatRect(dstRect.x(), dstRect.y() + dstRect.height() - bottom, left, bottom), FloatRect(srcRect.x(), srcRect.y() + srcRect.height() - bottom, left, bottom));
    // bottom-middle
    ctx.drawImage(image, FloatRect(dstRect.x() + left, dstRect.y() + dstRect.height() - bottom, dstRect.width() - left - right, bottom), FloatRect(srcRect.x() + left, srcRect.y() + srcRect.height() - bottom, srcRect.width() - left - right, bottom));
    // bottom-right
    ctx.drawImage(image, FloatRect(dstRect.x() + dstRect.width() - right, dstRect.y() + dstRect.height() - bottom, right, bottom), FloatRect(srcRect.x() + srcRect.width() - right, srcRect.y() + srcRect.height() - bottom, right, bottom));
}

bool RenderThemePlayStation::supportsHover(const RenderStyle&) const
{
    return true;
}

bool RenderThemePlayStation::supportsFocusRing(const RenderStyle&) const
{
    return false;
}

bool RenderThemePlayStation::paintCheckbox(const RenderObject& obj, const PaintInfo& info, const FloatRect& rect)
{
    static Image& onNormal = loadThemeImage("theme/checkbox/checkbox_on_normal.png").leakRef();
    static Image& onFocus = loadThemeImage("theme/checkbox/checkbox_on_focus.png").leakRef();
    static Image& onHover = loadThemeImage("theme/checkbox/checkbox_on_hover.png").leakRef();
    static Image& offNormal = loadThemeImage("theme/checkbox/checkbox_off_normal.png").leakRef();
    static Image& offFocus = loadThemeImage("theme/checkbox/checkbox_off_focus.png").leakRef();
    static Image& offHover = loadThemeImage("theme/checkbox/checkbox_off_hover.png").leakRef();

    if (this->isChecked(obj))
        paintThemeImage(info.context(), isFocused(obj) ? onFocus : isHovered(obj) ? onHover : onNormal, rect);
    else
        paintThemeImage(info.context(), isFocused(obj) ? offFocus : isHovered(obj) ? offHover : offNormal, rect);

    return false;
}

void RenderThemePlayStation::setCheckboxSize(RenderStyle& style) const
{
    // width and height both specified so do nothing
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    // hardcode size to 13 to match Firefox
    if (style.width().isIntrinsicOrAuto())
        style.setWidth(Length(13, LengthType::Fixed));

    if (style.height().isAuto())
        style.setHeight(Length(13, LengthType::Fixed));
}

bool RenderThemePlayStation::paintRadio(const RenderObject& obj, const PaintInfo& info, const FloatRect& rect)
{
    static Image& onNormal = loadThemeImage("theme/radio/radio_on_normal.png").leakRef();
    static Image& onFocus = loadThemeImage("theme/radio/radio_on_focus.png").leakRef();
    static Image& onHover = loadThemeImage("theme/radio/radio_on_hover.png").leakRef();
    static Image& offNormal = loadThemeImage("theme/radio/radio_off_normal.png").leakRef();
    static Image& offFocus = loadThemeImage("theme/radio/radio_off_focus.png").leakRef();
    static Image& offHover = loadThemeImage("theme/radio/radio_off_hover.png").leakRef();

    if (isChecked(obj))
        paintThemeImage(info.context(), isFocused(obj) ? onFocus : isHovered(obj) ? onHover : onNormal, rect);
    else
        paintThemeImage(info.context(), isFocused(obj) ? offFocus : isHovered(obj) ? offHover : offNormal, rect);

    return false;
}

void RenderThemePlayStation::setRadioSize(RenderStyle& style) const
{
    // width and height both specified so do nothing
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    // hardcode size to 13 to match Firefox
    if (style.width().isIntrinsicOrAuto())
        style.setWidth(Length(13, LengthType::Fixed));

    if (style.height().isAuto())
        style.setHeight(Length(13, LengthType::Fixed));
}

void RenderThemePlayStation::adjustButtonStyle(RenderStyle& style, const Element*) const
{
    // ignore line-height
    if (style.appearance() == StyleAppearance::PushButton)
        style.setLineHeight(RenderStyle::initialLineHeight());
}

bool RenderThemePlayStation::paintButton(const RenderObject& obj, const PaintInfo& info, const IntRect& rect)
{
    static Image& normal = loadThemeImage("theme/button/button_normal.png").leakRef();
    static Image& press = loadThemeImage("theme/button/button_press.png").leakRef();
    static Image& focus = loadThemeImage("theme/button/button_focus.png").leakRef();
    static Image& hover = loadThemeImage("theme/button/button_hover.png").leakRef();

    paintThemeImage3x3(info.context(), isPressed(obj) ? press : isFocused(obj) ? focus : isHovered(obj) ? hover : normal, rect, 6, 6, 6, 6);

    return false;
}

void RenderThemePlayStation::adjustMenuListStyle(RenderStyle& style, const Element*) const
{
    // The tests check explicitly that select menu buttons ignore line height.
    style.setLineHeight(RenderStyle::initialLineHeight());

    // We cannot give a proper rendering when border radius is active, unfortunately.
    style.resetBorderRadius();
}

bool RenderThemePlayStation::paintMenuList(const RenderObject& obj, const PaintInfo& info, const FloatRect& rect)
{
    static Image& normal = loadThemeImage("theme/menulist/menulist_normal.png").leakRef();
    static Image& press = loadThemeImage("theme/menulist/menulist_press.png").leakRef();
    static Image& focus = loadThemeImage("theme/menulist/menulist_focus.png").leakRef();
    static Image& hover = loadThemeImage("theme/menulist/menulist_hover.png").leakRef();

    paintThemeImage3x3(info.context(), isPressed(obj) ? press : isFocused(obj) ? focus : isHovered(obj) ? hover : normal, rect, 6, 20, 6, 6);

    paintMenuListButtonDecorations(downcast<RenderBox>(obj), info, rect);
    return false;
}

void RenderThemePlayStation::adjustMenuListButtonStyle(RenderStyle& style, const Element* element) const
{
    adjustMenuListStyle(style, element);
}

void RenderThemePlayStation::paintMenuListButtonDecorations(const RenderBox& box, const PaintInfo& paintInfo, const FloatRect& r)
{
    IntRect bounds = IntRect(r.x() + box.style().borderLeftWidth(),
        r.y() + box.style().borderTopWidth(),
        r.width() - box.style().borderLeftWidth() - box.style().borderRightWidth(),
        r.height() - box.style().borderTopWidth() - box.style().borderBottomWidth());
    const float arrowHeight = 5;
    const float arrowWidth = 7;
    const int arrowPaddingLeft = 5;
    const int arrowPaddingRight = 5;

    // Since we actually know the size of the control here, we restrict the font scale to make sure the arrow will fit vertically in the bounds
    float centerY = bounds.y() + bounds.height() / 2.0f;
    float leftEdge = bounds.maxX() - arrowPaddingRight - arrowWidth;
    float buttonWidth = arrowPaddingLeft + arrowWidth + arrowPaddingRight;

    if (bounds.width() < arrowWidth + arrowPaddingLeft)
        return;

    GraphicsContext& context = paintInfo.context();
    context.save();

    // Draw button background
    uint8_t fgGray = isPressed(box) ? 247 : isHovered(box) ? 84 : 94;
    uint8_t bgGray = isPressed(box) ? 84 : isHovered(box) ? 216 : 226;
    Color fgColor = SRGBA<uint8_t> { fgGray, fgGray, fgGray };
    Color bgColor = SRGBA<uint8_t> { bgGray, bgGray, bgGray };
    context.setStrokeStyle(StrokeStyle::SolidStroke);
    context.setStrokeColor(fgColor);
    context.setFillColor(bgColor);
    FloatRect button(bounds.maxX() - buttonWidth, bounds.y(), buttonWidth, bounds.height());
    button.inflate(-2);
    context.fillRect(button);
    context.strokeRect(button, 1);

    // Draw the arrow
    context.setFillColor(fgColor);
    context.setStrokeStyle(StrokeStyle::NoStroke);

    Vector<FloatPoint> arrow =
    {
        FloatPoint(leftEdge, centerY - arrowHeight / 2.0f),
        FloatPoint(leftEdge + arrowWidth, centerY - arrowHeight / 2.0f),
        FloatPoint(leftEdge + arrowWidth / 2.0f, centerY + arrowHeight / 2.0f)
    };

    context.fillPath(Path::polygonPathFromPoints(arrow));
    context.restore();
}

bool RenderThemePlayStation::paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&)
{
    return true;
}


bool RenderThemePlayStation::paintSliderTrack(const RenderObject& obj, const PaintInfo& info, const IntRect& rect)
{
    const Color activeColor = SRGBA<uint8_t> { 0x00, 0x70, 0xCC };
    const Color trackColor = SRGBA<uint8_t> { 0xD8, 0xD8, 0xD8 };
    const float thickness = 3;

    FloatRect baseRect = rect;
    if (obj.style().effectiveAppearance() == StyleAppearance::SliderHorizontal) {
        baseRect.setHeight(thickness);
        baseRect.move(0, (rect.height() - baseRect.height()) / 2);
    } else if (obj.style().effectiveAppearance() == StyleAppearance::SliderVertical) {
        baseRect.setWidth(thickness);
        baseRect.move((rect.width() - baseRect.width()) / 2, 0);
    }

    float radius = thickness / 2;
    FloatRoundedRect roundedRect(baseRect, FloatRoundedRect::Radii(radius));

    LayoutPoint activeLocation;
    LayoutUnit margin;
    if (is<HTMLInputElement>(obj.node())) {
        auto& input = downcast<HTMLInputElement>(*obj.node());
        if (auto* element = input.sliderThumbElement()) {
            activeLocation = element->renderBox()->location();
            margin = element->renderBox()->width() / 2; // Width and height in SliderThumb are the same size
        }
    }

    FloatRect activeRect = baseRect;

    if (obj.style().effectiveAppearance() == StyleAppearance::SliderHorizontal) {
        if (obj.style().direction() == TextDirection::RTL) {
            activeRect.setWidth(baseRect.width() - activeLocation.x() - margin);
            activeRect.move(baseRect.width() - activeRect.width(), 0);
        } else
            activeRect.setWidth(activeLocation.x() + margin);
    }  else if (obj.style().effectiveAppearance() == StyleAppearance::SliderVertical)
        activeRect.setHeight(activeLocation.y() + margin);

    GraphicsContext& context = info.context();
    context.save();
    context.clipRoundedRect(roundedRect);
    context.fillRect(baseRect, trackColor);
    context.fillRect(activeRect, activeColor);
    context.restore();

    return false;
}

bool RenderThemePlayStation::paintSliderThumb(const RenderObject& obj, const PaintInfo& info, const IntRect& rect)
{
    const Color outlineNormalColor = SRGBA<uint8_t> { 0x97, 0x97, 0x97 };
    const Color baseNormalColor = SRGBA<uint8_t> { 0xD8, 0xD8, 0xD8 };

    const Color outlineFocusedColor = SRGBA<uint8_t> { 0x7E, 0x7E, 0x7E };
    const Color baseFocusedColor = SRGBA<uint8_t> { 0xB0, 0xB0, 0xB0 };

    FloatRect baseRect = rect;
    float radius = baseRect.width() / 2;
    FloatRoundedRect outlineRect(baseRect, FloatRoundedRect::Radii(radius));

    const float outlineThickness = 1;
    baseRect.inflate(-outlineThickness);
    radius = baseRect.width() / 2;
    FloatRoundedRect pointerRect(baseRect, FloatRoundedRect::Radii(radius));

    Color outlineColor;
    Color baseColor;
    if (isHovered(obj) || isFocused(obj)) {
        outlineColor = outlineFocusedColor;
        baseColor = baseFocusedColor;
    } else {
        outlineColor = outlineNormalColor;
        baseColor = baseNormalColor;
    }

    GraphicsContext& context = info.context();
    context.save();
    context.fillRoundedRect(outlineRect, outlineColor);
    context.fillRoundedRect(pointerRect, baseColor);
    context.restore();

    return false;
}

const int sliderThumbWidth = 15;
const int sliderThumbHeight = 15;

void RenderThemePlayStation::adjustSliderThumbSize(RenderStyle& style, const Element*) const
{
    StyleAppearance appearance = style.effectiveAppearance();
    switch (appearance) {
    case StyleAppearance::SliderThumbVertical:
        style.setWidth(Length(sliderThumbHeight, LengthType::Fixed));
        style.setHeight(Length(sliderThumbWidth, LengthType::Fixed));
        break;
    case StyleAppearance::SliderThumbHorizontal:
        style.setWidth(Length(sliderThumbWidth, LengthType::Fixed));
        style.setHeight(Length(sliderThumbHeight, LengthType::Fixed));
        break;
    default:
        break;
    }
}

bool RenderThemePlayStation::paintProgressBar(const RenderObject& renderObject, const PaintInfo& info, const IntRect& rect)
{
    const float xMarginRatio = 1.0 / 8;
    const float yMarginRatio = 3.0 / 16;
    const Color backgroundColor = SRGBA<uint8_t> {0xCD, 0xCD, 0xCD};
    const Color backgroundColorIndeterminateOdd = SRGBA<uint8_t> {0x88, 0x88, 0x88};
    const Color backgroundColorIndeterminateEven = SRGBA<uint8_t> {0x9B, 0x9B, 0x9B};
    const Color foregroundColor = SRGBA<uint8_t> {0x00, 0x70, 0xCC};

    // Calc margin
    FloatRect backgroundRect = rect;
    backgroundRect.inflateX(rect.height() * -xMarginRatio);
    backgroundRect.inflateY(rect.height() * -yMarginRatio);

    float radius = backgroundRect.height() / 2;

    FloatRoundedRect roundedBackgroundRect(backgroundRect, FloatRoundedRect::Radii(radius));

    auto& context = info.context();
    context.save();
    context.clipRoundedRect(roundedBackgroundRect);

    const auto& renderProgress = downcast<RenderProgress>(renderObject);

    if (renderProgress.isDeterminate()) {
        context.fillRect(backgroundRect, backgroundColor);

        FloatRect foregroundRect = backgroundRect;
        auto progressWidth = foregroundRect.width() * renderProgress.position();
        if (renderObject.style().direction() == TextDirection::RTL)
            foregroundRect.move(foregroundRect.width() - progressWidth, 0);
        foregroundRect.setWidth(progressWidth);
        context.fillRect(foregroundRect, foregroundColor);
    } else {
        // Striped pattern
        // repeat width
        float gapWidth = backgroundRect.height() / 2;
        int stripesCount = backgroundRect.width() / gapWidth / 2 + 1;

        // Striped base rect
        FloatRect baseRect = backgroundRect;
        baseRect.setWidth(gapWidth);

        // Odd numbered areas
        FloatRect oddRect = baseRect;
        for (int i = 0; i < stripesCount; i++) {
            context.fillRect(oddRect, backgroundColorIndeterminateOdd);
            oddRect.setX(oddRect.x() + gapWidth * 2);
        }

        // Even numbered areas
        FloatRect evenRect = baseRect;
        evenRect.setX(evenRect.x() + gapWidth);
        for (int i = 0; i < stripesCount; i++) {
            context.fillRect(evenRect, backgroundColorIndeterminateEven);
            evenRect.setX(evenRect.x() + gapWidth * 2);
        }
    }

    context.restore();
    return false;
}

bool RenderThemePlayStation::supportsMeter(StyleAppearance appearance, const HTMLMeterElement&) const
{
    switch (appearance) {
    case StyleAppearance::Meter:
        return true;
    default:
        return false;
    }
}

bool RenderThemePlayStation::paintMeter(const RenderObject& renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    if (!is<RenderMeter>(renderObject))
        return true;

    const Color baseColor = SRGBA<uint8_t> {0xCD, 0xCD, 0xCD};
    const Color optimumColor = SRGBA<uint8_t> {0x38, 0xCD, 0x03};
    const Color suboptimalColor = SRGBA<uint8_t> {0xEC, 0xB6, 0x08};
    const Color evenLessGoodColor = SRGBA<uint8_t> {0xCD, 0x03, 0x1B};
    float marginRatioY = 3.0 / 16;

    FloatRect baseRect = rect;
    baseRect.inflateY(baseRect.height() * -marginRatioY);
    float radius = baseRect.height() / 2;
    FloatRoundedRect roundedRect(baseRect, FloatRoundedRect::Radii(radius));

    HTMLMeterElement* element = downcast<RenderMeter>(renderObject).meterElement();

    FloatRect valueRect = baseRect;

    valueRect.setWidth(valueRect.width() * element->valueRatio());
    if (renderObject.style().direction() == TextDirection::RTL)
        valueRect.move(baseRect.width() - valueRect.width(), 0);

    Color valueColor;
    switch (element->gaugeRegion()) {
    case HTMLMeterElement::GaugeRegionOptimum:
        valueColor = optimumColor;
        break;
    case HTMLMeterElement::GaugeRegionSuboptimal:
        valueColor = suboptimalColor;
        break;
    case HTMLMeterElement::GaugeRegionEvenLessGood:
        valueColor = evenLessGoodColor;
        break;
    default:
        valueColor = baseColor;
    }

    auto& context = paintInfo.context();
    context.save();
    context.clipRoundedRect(roundedRect);
    context.fillRect(baseRect, baseColor);
    context.fillRect(valueRect, valueColor);
    context.restore();

    return true;
}

LengthBox RenderThemePlayStation::popupInternalPaddingBox(const RenderStyle&, const Settings&) const
{
    return { 3,  5 + 20 /* offset 20 for menulist button icon */ , 3, 5};
}

Color RenderThemePlayStation::platformFocusRingColor(OptionSet<StyleColorOptions>) const
{
    return SRGBA<uint8_t> { 0xD9, 0xAD, 0x7D };
}

Color RenderThemePlayStation::platformActiveSelectionBackgroundColor(OptionSet<StyleColorOptions>) const
{
    return SRGBA<uint8_t> { 0x00, 0x70, 0xCC, 0x4D };
}

Color RenderThemePlayStation::platformActiveSelectionForegroundColor(OptionSet<StyleColorOptions>) const
{
    return { };
}

Color RenderThemePlayStation::platformInactiveSelectionBackgroundColor(OptionSet<StyleColorOptions>) const
{
    return SRGBA<uint8_t> { 0x6C, 0x6C, 0x6C, 0x4D };
}

Color RenderThemePlayStation::platformInactiveSelectionForegroundColor(OptionSet<StyleColorOptions>) const
{
    return { };
}

Color RenderThemePlayStation::platformActiveListBoxSelectionBackgroundColor(OptionSet<StyleColorOptions> options) const
{
    return platformActiveSelectionBackgroundColor(options);
}

Color RenderThemePlayStation::platformActiveListBoxSelectionForegroundColor(OptionSet<StyleColorOptions>) const
{
    return Color::black;
}

Color RenderThemePlayStation::platformInactiveListBoxSelectionBackgroundColor(OptionSet<StyleColorOptions> options) const
{
    return platformInactiveSelectionBackgroundColor(options);
}

Color RenderThemePlayStation::platformInactiveListBoxSelectionForegroundColor(OptionSet<StyleColorOptions>) const
{
    return Color::black;
}

#if ENABLE(VIDEO)
String RenderThemePlayStation::mediaControlsStyleSheet()
{
#if ENABLE(MEDIA_SOURCE)
    return String(mediaControlsPlayStationUserAgentStyleSheet, sizeof(mediaControlsPlayStationUserAgentStyleSheet));
#else
    return String(mediaControlsPlayStationSimpleUserAgentStyleSheet, sizeof(mediaControlsPlayStationSimpleUserAgentStyleSheet));
#endif
}

Vector<String, 2> RenderThemePlayStation::mediaControlsScripts()
{
    auto controlsScript = String(mediaControlsLocalizedStringsJavaScript, sizeof(mediaControlsLocalizedStringsJavaScript));
#if ENABLE(MEDIA_SOURCE)
    controlsScript = controlsScript + String(mediaControlsPlayStationJavaScript, sizeof(mediaControlsPlayStationJavaScript));
#else
    controlsScript = controlsScript + String(mediaControlsPlayStationSimpleJavaScript, sizeof(mediaControlsPlayStationSimpleJavaScript));
#endif
    return { controlsScript };
}
#endif
} // namespace WebCore
