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
#include "WKImeEvent.h"

#include <unicode/uloc.h>

struct InputLanguageEntry {
    const char* attr;
    WKInputLanguage value;
};

static const InputLanguageEntry languageEntry[] = {
    // commonLanguage
    { "da_Latn_DK",         INPUT_LANGUAGE_DANISH },
    { "de_Latn_DE",         INPUT_LANGUAGE_GERMAN },
    { "en_Latn_US",         INPUT_LANGUAGE_ENGLISH_US },
    { "es_Latn_ES",         INPUT_LANGUAGE_SPANISH },
    { "fr_Latn_FR",         INPUT_LANGUAGE_FRENCH },
    { "it_Latn_IT",         INPUT_LANGUAGE_ITALIAN },
    { "nl_Latn_NL",         INPUT_LANGUAGE_DUTCH },
    { "no_Latn_NO",         INPUT_LANGUAGE_NORWEGIAN },
    { "pl_Latn_PL",         INPUT_LANGUAGE_POLISH },
    { "pt_Latn_BR",         INPUT_LANGUAGE_PORTUGUESE_BR },
    { "ru_Cyrl_RU",         INPUT_LANGUAGE_RUSSIAN },
    { "fi_Latn_FI",         INPUT_LANGUAGE_FINNISH },
    { "se_Latn_NO",         INPUT_LANGUAGE_SWEDISH },
    { "sv_Latn_SE",         INPUT_LANGUAGE_SWEDISH },
    { "ja_Jpan_JP",         INPUT_LANGUAGE_JAPANESE },
    { "ko_Kore_KR",         INPUT_LANGUAGE_KOREAN },
    { "zh_Hans_CN",         INPUT_LANGUAGE_SIMPLIFIED_CHINESE },
    { "tr_Latn_TR",         INPUT_LANGUAGE_TURKISH },
    { "ar_Arab_EG",         INPUT_LANGUAGE_ARABIC },
    { "th_Thai_TH",         INPUT_LANGUAGE_THAI },
    { "cs_Latn_CZ",         INPUT_LANGUAGE_CZECH },
    { "el_Grek_GR",         INPUT_LANGUAGE_GREEK },
    { "id_Latn_ID",         INPUT_LANGUAGE_INDONESIAN },
    { "vi_Latn_VN",         INPUT_LANGUAGE_VIETNAMESE },
    { "ro_Latn_RO",         INPUT_LANGUAGE_ROMANIAN },
    { "hu_Latn_HU",         INPUT_LANGUAGE_HUNGARIAN },
    { "uk_Cyrl_UA",         INPUT_LANGUAGE_UKRAINIAN },
    // dialectLanguage
    { "en_Latn_GB",         INPUT_LANGUAGE_ENGLISH_GB },
    { "es_Latn_419",        INPUT_LANGUAGE_SPANISH_LATIN_AMERICA },
    { "pt_Latn_PT",         INPUT_LANGUAGE_PORTUGUESE_PT },
    { "zh_Hant_HK",         INPUT_LANGUAGE_TRADITIONAL_CHINESE },
    { "zh_Hant_TW",         INPUT_LANGUAGE_TRADITIONAL_CHINESE },
    { "fr_Latn_CA",         INPUT_LANGUAGE_CANADIAN_FRENCH },
};

WKInputLanguage WKImeEventGetInputLanguage(const char* languageCode)
{
    UErrorCode status = U_ZERO_ERROR;
    char maximizedLocale[ULOC_FULLNAME_CAPACITY];
    int32_t maximizedLocaleLength = uloc_addLikelySubtags(languageCode, maximizedLocale, ULOC_FULLNAME_CAPACITY, &status);

    if (U_FAILURE(status))
        return InputLanguageUndefined;

    for (unsigned i = 0; i < (sizeof(languageEntry) / sizeof(languageEntry[0])); i++) {
        const char* locale = languageEntry[i].attr;
        if (!strncmp(maximizedLocale, locale, std::max(maximizedLocaleLength, (int32_t)strlen(locale))))
            return languageEntry[i].value;
    }
    return InputLanguageUndefined;
}

WKImeEvent* WKImeEventMake(WKImeEventType imeEventType, const char* compositionString, int keyCode)
{
    WKImeEvent* imeEvent = new WKImeEvent;
    imeEvent->imeEventType = imeEventType;
    imeEvent->compositionString = compositionString;
    imeEvent->keyCode = keyCode;
    imeEvent->caretIndex = 0;
    if (compositionString) {
        int len = strlen(compositionString);
        wchar_t* buf = new wchar_t[len + 1];
        size_t returnValue = 0;
        mbstowcs_s(&returnValue, buf, len + 1, compositionString, len);
        imeEvent->caretIndex = returnValue;
        delete [] buf;
    }
    return imeEvent;
}

void WKImeEventRelease(WKImeEvent* imeEvent)
{
    delete imeEvent;
}
