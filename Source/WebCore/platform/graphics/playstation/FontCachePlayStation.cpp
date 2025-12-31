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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FontCache.h"

#include "Font.h"
#include "UTF16UChar32Iterator.h"
#include <cairo-ft.h>

namespace WebCore {

static int fontWeightToFontconfigWeight(FontSelectionValue weight)
{
    if (weight < FontSelectionValue(350))
        return FC_WEIGHT_LIGHT;
    if (weight < FontSelectionValue(450))
        return FC_WEIGHT_REGULAR;
    return FC_WEIGHT_MEDIUM;
}

static bool configurePatternForFontDescription(FcPattern* pattern, const FontDescription& fontDescription)
{
    if (!FcPatternAddInteger(pattern, FC_SLANT, fontDescription.italic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN))
        return false;
    if (!FcPatternAddInteger(pattern, FC_WEIGHT, fontWeightToFontconfigWeight(fontDescription.weight())))
        return false;
    return true;
}

static RefPtr<FcPattern> systemFallbackFontPatternForCharacters(const FontDescription& description, const UChar* characters, unsigned length)
{
    FcUniquePtr<FcCharSet> fontConfigCharSet(FcCharSetCreate());
    UTF16UChar32Iterator iterator(characters, length);
    UChar32 character = iterator.next();
    while (character != iterator.end()) {
        FcCharSetAddChar(fontConfigCharSet.get(), character);
        character = iterator.next();
    }

    RefPtr<FcPattern> pattern = adoptRef(FcPatternCreate());
    FcPatternAddCharSet(pattern.get(), FC_CHARSET, fontConfigCharSet.get());

    FcPatternAddBool(pattern.get(), FC_SCALABLE, FcTrue);

    if (!configurePatternForFontDescription(pattern.get(), description))
        return nullptr;

    FcConfigSubstitute(nullptr, pattern.get(), FcMatchPattern);
    FcDefaultSubstitute(pattern.get());

    FcResult fontConfigResult;
    return adoptRef(FcFontMatch(nullptr, pattern.get(), &fontConfigResult));
}

struct FontFaceCacheKeyHash {
    static unsigned hash(const RefPtr<FcPattern> pattern)
    {
        ASSERT(pattern);
        StringHasher hasher;

        char* filePath;
        if (FcPatternGetString(pattern.get(), FC_FILE, 0, (FcChar8**)&filePath) == FcResultMatch)
            hasher.addCharacters(filePath, strlen(filePath));

        int slant;
        if (FcPatternGetInteger(pattern.get(), FC_SLANT, 0, &slant) == FcResultMatch)
            hasher.addCharacters((char*)&slant, sizeof slant);

        int weight;
        if (FcPatternGetInteger(pattern.get(), FC_WEIGHT, 0, &weight) == FcResultMatch)
            hasher.addCharacters((char*)&weight, sizeof weight);

        return hasher.hash();
    }

    static bool equal(const RefPtr<FcPattern> a, const RefPtr<FcPattern> b)
    {
        ASSERT(a);
        ASSERT(b);
        return FcPatternEqual(a.get(), b.get());
    }

    static const bool safeToCompareToEmptyOrDeleted = false;
};

typedef HashMap<RefPtr<FcPattern>, RefPtr<cairo_font_face_t>, FontFaceCacheKeyHash> FontFaceCache;

static FontFaceCache& cachedFaces()
{
    static NeverDestroyed<FontFaceCache> cache;
    return cache;
}

RefPtr<Font> FontCache::systemFallbackForCharacters(const FontDescription& description, const Font&, IsForPlatformFont, PreferColoredFont, const UChar* characters, unsigned length)
{
    auto pattern = systemFallbackFontPatternForCharacters(description, characters, length);
    if (!pattern)
        return lastResortFallbackFont(description);

    auto addResult = cachedFaces().add(pattern, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = adoptRef(cairo_ft_font_face_create_for_pattern(pattern.get()));

    // We don't use synthetic bold.
    bool syntheticBold = false;

    // We don't have italic fonts so far. Use syntheticOblique if italic is requested.
    bool syntheticOblique = (description.hasAutoFontSynthesisStyle()) && description.italic();

    bool isFixedWidth = false; // FIXME: We may have fixed width fonts.

    FontPlatformData alternateFontData(addResult.iterator->value.get(), addResult.iterator->key.get(), description.computedSize(), isFixedWidth, syntheticBold, syntheticOblique, description.orientation());
    return fontForPlatformData(alternateFontData);
}

}
