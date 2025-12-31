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

#pragma once

#include "RenderTheme.h"

namespace WebCore {

class RenderThemePlayStation final : public RenderTheme {
public:
    friend NeverDestroyed<RenderThemePlayStation>;

private:

#if ENABLE(VIDEO)
    String mediaControlsStyleSheet() override;
    Vector<String, 2> mediaControlsScripts() final;
#endif

    static void paintThemeImage(GraphicsContext&, Image&, const FloatRect&);
    static void paintThemeImage3x3(GraphicsContext&, Image&, const FloatRect&, int left = 0, int right = 0, int top = 0, int bottom = 0);

    // A method asking if the theme is able to draw the focus ring.
    bool supportsFocusRing(const RenderStyle&) const override;

    // A method asking if the theme's controls actually care about redrawing when hovered.
    bool supportsHover(const RenderStyle&) const override;

    // Methods for each appearance value.
    bool paintCheckbox(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    void setCheckboxSize(RenderStyle&) const override;

    bool paintRadio(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    void setRadioSize(RenderStyle&) const override;

    void adjustButtonStyle(RenderStyle&, const Element*) const override;
    bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    void adjustMenuListStyle(RenderStyle&, const Element*) const override;
    bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    void adjustMenuListButtonStyle(RenderStyle&, const Element*) const override;
    void paintMenuListButtonDecorations(const RenderBox&, const PaintInfo&, const FloatRect&) override;

    bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;
    void adjustSliderThumbSize(RenderStyle&, const Element*) const override;

    bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;

    bool supportsMeter(StyleAppearance, const HTMLMeterElement&) const override;
    bool paintMeter(const RenderObject&, const PaintInfo&, const IntRect&) override;

    LengthBox popupInternalPaddingBox(const RenderStyle&, const Settings&) const override;

    Color platformFocusRingColor(OptionSet<StyleColorOptions>) const override;
    Color platformActiveSelectionBackgroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformInactiveSelectionBackgroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformActiveSelectionForegroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformInactiveSelectionForegroundColor(OptionSet<StyleColorOptions>) const override;

    Color platformActiveListBoxSelectionBackgroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformInactiveListBoxSelectionBackgroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformActiveListBoxSelectionForegroundColor(OptionSet<StyleColorOptions>) const override;
    Color platformInactiveListBoxSelectionForegroundColor(OptionSet<StyleColorOptions>) const override;

    bool popsMenuBySpaceOrReturn() const override { return true; }

private:
    RenderThemePlayStation();
    virtual ~RenderThemePlayStation();
};

} // namespace WebCore
