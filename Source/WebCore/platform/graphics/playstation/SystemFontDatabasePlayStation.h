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

#pragma once

#include "FcUniquePtr.h"
#include "FontCascadeDescription.h"
#include "RefPtrFontConfig.h"
#include "SystemFontDatabase.h"
#include <cairo-ft.h>
#include <wtf/HashMap.h>
#include <wtf/HashTraits.h>
#include <wtf/Language.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/AtomStringHash.h>

namespace WebCore {

class SystemFontDatabasePlayStation : public SystemFontDatabase {
public:
    struct CascadeListParameters {
        CascadeListParameters()
        {
        }

        CascadeListParameters(WTF::HashTableDeletedValueType)
            : fontName(WTF::HashTableDeletedValue)
        {
        }

        bool isHashTableDeletedValue() const
        {
            return fontName.isHashTableDeletedValue();
        }

        bool operator==(const CascadeListParameters& other) const
        {
            return fontName == other.fontName
                && language == other.language
                && weight == other.weight
                && size == other.size
                && slant == other.slant;
        }

        unsigned hash() const
        {
            return computeHash(fontName, language, weight, size, slant);
        }

        struct Hash {
            static unsigned hash(const CascadeListParameters& parameters) { return parameters.hash(); }
            static bool equal(const CascadeListParameters& a, const CascadeListParameters& b) { return a == b; }
            static const bool safeToCompareToEmptyOrDeleted = true;
        };

        struct CascadeListParametersHashTraits : SimpleClassHashTraits<CascadeListParameters> {
            static const bool hasIsEmptyValueFunction = true;
            static bool isEmptyValue(const CascadeListParameters& value)
            {
                return value.fontName.isNull() || value.fontName.isEmpty();
            }
        };

        AtomString fontName;
        AtomString language;
        int weight { FC_WEIGHT_REGULAR };
        float size { 0 };
        int slant { FC_SLANT_ROMAN };
    };

    static SystemFontDatabasePlayStation& singleton();

    Vector<FontFamilySpecificationPlayStation> cascadeList(const FontCascadeDescription&, const AtomString& cssFamily);

    void clear()
    {
        m_systemFontCache.clear();
    }

private:
    SystemFontDatabasePlayStation()
    {
        // Ensure FcInit() is called before computeCascadeList().
        // It's fine to call FcInit multiple times per the documentation.
        if (!FcInit())
            ASSERT_NOT_REACHED();
    }

    static Vector<FontFamilySpecificationPlayStation> computeCascadeList(const CascadeListParameters&);

    HashMap<CascadeListParameters, Vector<FontFamilySpecificationPlayStation>, CascadeListParameters::Hash, CascadeListParameters::CascadeListParametersHashTraits> m_systemFontCache;
};

}
