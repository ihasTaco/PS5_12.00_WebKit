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
#include "FontDescription.h"

#include "FontCache.h"
#include "FontFamilySpecificationPlayStation.h"
#include "SystemFontDatabasePlayStation.h"

namespace WebCore {

Vector<FontFamilySpecificationPlayStation> systemFontCascadeList(const FontCascadeDescription& description, const AtomString& cssFamily)
{
    return SystemFontDatabasePlayStation::singleton().cascadeList(description, cssFamily);
}

static inline bool isSystemFontString(const AtomString& string)
{
    return equalLettersIgnoringASCIICase(string, "-webkit-standard"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-sans-serif"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-serif"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-cursive"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-monospace"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-fantasy"_s)
        || equalLettersIgnoringASCIICase(string, "-webkit-pictograph"_s);
    return false;
}

unsigned FontCascadeDescription::effectiveFamilyCount() const
{
    unsigned result = 0;
    for (unsigned i = 0; i < familyCount(); ++i) {
        const auto& cssFamily = familyAt(i);
        if (isSystemFontString(cssFamily))
            result += systemFontCascadeList(*this, cssFamily).size();
        else
            ++result;
    }
    return result;
}

FontFamilySpecification FontCascadeDescription::effectiveFamilyAt(unsigned index) const
{
    for (unsigned i = 0; i < familyCount(); ++i) {
        const auto& cssFamily = familyAt(i);
        if (isSystemFontString(cssFamily)) {
            auto cascadeList = systemFontCascadeList(*this, cssFamily);
            if (index < cascadeList.size())
                return cascadeList[index];
            index -= cascadeList.size();
        } else if (!index)
            return cssFamily;
        else
            --index;
    }
    ASSERT_NOT_REACHED();
    return nullAtom();
}

}
