/*
 * Copyright (C) 2023 Sony Interactive Entertainment Inc.
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
#include "SystemFontDatabasePlayStation.h"

#include "NotImplemented.h"
#include "WebKitFontFamilyNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

SystemFontDatabasePlayStation& SystemFontDatabasePlayStation::singleton()
{
    static NeverDestroyed<SystemFontDatabasePlayStation> database = SystemFontDatabasePlayStation();
    return database.get();
}

SystemFontDatabase& SystemFontDatabase::singleton()
{
    return SystemFontDatabasePlayStation::singleton();
}

auto SystemFontDatabase::platformSystemFontShorthandInfo(FontShorthand fontShorthand) -> SystemFontShorthandInfo
{
    float fontSize = 16;

    switch (fontShorthand) {
    case FontShorthand::WebkitMiniControl:
    case FontShorthand::WebkitSmallControl:
    case FontShorthand::WebkitControl:
        // Why 2 points smaller? Because that's what Gecko does. Note that we
        // are assuming a 96dpi screen, which is the default that we use on
        // Windows.
        static const float pointsPerInch = 72.0f;
        static const float pixelsPerInch = 96.0f;
        fontSize -= (2.0f / pointsPerInch) * pixelsPerInch;
        break;
    default:
        break;
    }
    return { WebKitFontFamilyNames::standardFamily, fontSize, normalWeightValue() };
}

static int fontWeightToFontconfigWeight(FontSelectionValue weight)
{
    if (weight < FontSelectionValue(350))
        return FC_WEIGHT_LIGHT;
    if (weight < FontSelectionValue(450))
        return FC_WEIGHT_REGULAR;
    return FC_WEIGHT_MEDIUM;
}

static int italicToFontconfigSlant(bool italic)
{
    return italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN;
}

static bool isCJK(const WTF::AtomString& locale)
{
    if (locale.isEmpty())
        return false;

    if (equalIgnoringASCIICase(locale, "ja"_s)
        || equalIgnoringASCIICase(locale, "ko"_s)
        || equalIgnoringASCIICase(locale, "zh"_s)
        || locale.startsWithIgnoringASCIICase("ja-"_s)
        || locale.startsWithIgnoringASCIICase("ko-"_s)
        || locale.startsWithIgnoringASCIICase("zh-"_s))
        return true;

    return false;
}

static inline SystemFontDatabasePlayStation::CascadeListParameters systemFontParameters(const FontCascadeDescription& description, const AtomString& familyName)
{
    auto languages = userPreferredLanguages();
    auto locale = description.computedLocale();

    SystemFontDatabasePlayStation::CascadeListParameters result;
    result.size = description.computedSize();
    result.slant = italicToFontconfigSlant(isItalic(description.italic()));
    result.weight = fontWeightToFontconfigWeight(description.weight());
    result.fontName = familyName;

    // If the lang attribute is one of CJK, the font selection will prioritize the lang attribute over the system setting.
    if (isCJK(locale))
        result.language = locale;
    else if (!languages.isEmpty())
        result.language = AtomString{languages.at(0)};
    return result;
}

Vector<FontFamilySpecificationPlayStation> SystemFontDatabasePlayStation::cascadeList(const FontCascadeDescription& description, const AtomString& cssFamily)
{
    auto parameters = systemFontParameters(description, cssFamily);
    ASSERT(!parameters.fontName.isNull());
    return m_systemFontCache.ensure(parameters, [&] {
        auto result = computeCascadeList(parameters);
        ASSERT(!result.isEmpty());
        return result;
    }).iterator->value;
}

Vector<FontFamilySpecificationPlayStation> SystemFontDatabasePlayStation::computeCascadeList(const CascadeListParameters& parameters)
{
    // FIXME: Currently this function lists all the system fonts regardless of the given parameters.
    Vector<FontFamilySpecificationPlayStation> fonts;

    // Prepare pattern.
    RefPtr pattern { FcPatternCreate() };
    FcPatternAddString(pattern.get(), FC_FAMILY, (const FcChar8*)parameters.fontName.string().utf8().data());
    FcPatternAddInteger(pattern.get(), FC_WEIGHT, parameters.weight);
    FcPatternAddInteger(pattern.get(), FC_SLANT, parameters.slant);
    if (!parameters.language.isEmpty()) {
        FcLangSet* langSet = FcLangSetCreate();
        FcLangSetAdd(langSet, (const FcChar8*)parameters.language.string().utf8().data());
        FcPatternAddLangSet(pattern.get(), FC_LANG, langSet);
        FcLangSetDestroy(langSet);
    }

    FcUniquePtr<FcObjectSet> objectSet(FcObjectSetBuild(FC_FAMILY, FC_FILE, FC_MATRIX, FC_WEIGHT, FC_SLANT, "FCL_SYNTHETIC_WEIGHT", FC_EMBEDDED_BITMAP, nullptr));
    if (!objectSet)
        return fonts;

    // Building a font set (array of FcPattern).
    FcUniquePtr<FcFontSet> fontSet(FcFontList(nullptr, pattern.get(), objectSet.get()));
    if (!fontSet)
        return fonts;
    for (int i = 0; i < fontSet->nfont; i++) {
        RefPtr pattern { fontSet->fonts[i] };
        auto face = adoptRef(cairo_ft_font_face_create_for_pattern(fontSet->fonts[i]));
        fonts.append(FontFamilySpecificationPlayStation(WTFMove(face), WTFMove(pattern)));
    }

    return fonts;
}

void SystemFontDatabase::platformInvalidate()
{
}

} // namespace WebCore
