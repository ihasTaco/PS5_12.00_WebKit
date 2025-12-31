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


#pragma once

#include <WebKit/WKBase.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

enum WKInputFieldType {
    InputFieldTypeUndefined = -1,

    INPUT_FIELD_TEXT = 0,
    INPUT_FIELD_PASSWORD,
    INPUT_FIELD_EMAIL,
    INPUT_FIELD_URL,
    INPUT_FIELD_SEARCH,
    INPUT_FIELD_NUMBER,
    INPUT_FIELD_TELEPHONE,

    INPUT_FIELD_TEXTAREA, // TEXTAREA
    INPUT_FIELD_OTHER, // Any content-editable elements

    InputFieldTypeCount
};

enum WKInputLanguage {
    InputLanguageUndefined = -1,

    INPUT_LANGUAGE_DANISH,
    INPUT_LANGUAGE_GERMAN,
    INPUT_LANGUAGE_ENGLISH_US,
    INPUT_LANGUAGE_SPANISH,
    INPUT_LANGUAGE_FRENCH,
    INPUT_LANGUAGE_ITALIAN,
    INPUT_LANGUAGE_DUTCH,
    INPUT_LANGUAGE_NORWEGIAN,
    INPUT_LANGUAGE_POLISH,
    INPUT_LANGUAGE_PORTUGUESE_PT,
    INPUT_LANGUAGE_RUSSIAN,
    INPUT_LANGUAGE_FINNISH,
    INPUT_LANGUAGE_SWEDISH,
    INPUT_LANGUAGE_JAPANESE,
    INPUT_LANGUAGE_KOREAN,
    INPUT_LANGUAGE_SIMPLIFIED_CHINESE,
    INPUT_LANGUAGE_TRADITIONAL_CHINESE,
    INPUT_LANGUAGE_PORTUGUESE_BR,
    INPUT_LANGUAGE_ENGLISH_GB,
    INPUT_LANGUAGE_TURKISH,
    INPUT_LANGUAGE_SPANISH_LATIN_AMERICA,
    INPUT_LANGUAGE_ARABIC,
    INPUT_LANGUAGE_CANADIAN_FRENCH,
    INPUT_LANGUAGE_THAI,
    INPUT_LANGUAGE_CZECH,
    INPUT_LANGUAGE_GREEK,
    INPUT_LANGUAGE_INDONESIAN,
    INPUT_LANGUAGE_VIETNAMESE,
    INPUT_LANGUAGE_ROMANIAN,
    INPUT_LANGUAGE_HUNGARIAN,
    INPUT_LANGUAGE_UKRAINIAN,

    InputLanguageCount,
};

WK_EXPORT WKInputLanguage WKImeEventGetInputLanguage(const char* languageCode);

enum WKEditorStateEventType {
    None,
    ShouldImeOpen,
    ShouldImeClose,
    ShouldImeReopen,
    ShouldImeUpdateContext,
    ShouldImeUpdatePositionRect,
};

struct WKEditorState {
    WKEditorStateEventType eventType;
    WKInputFieldType type;
    WKInputLanguage lang;
    int caretOffset;
    const char* psOSKAttr;
    unsigned char contentEditable:1;
    unsigned char hasPreviousNode:1;
    unsigned char hasNextNode:1;
    unsigned char isInLoginForm:1;
    unsigned char canSubmitImplicitly:1;
};
typedef struct WKEditorState WKEditorState;

enum WKImeEventType {
    Open,
    ChangeSize,
    PressClose,
    PressEnter,
    UpdatePreedit,
    ConfirmPreedit,
    InsertText,
    Backspace,
    DeleteAllBeforeCaret,
    DeleteAll,
    CaretMoveLeft,
    CaretMoveRight,
    CaretMoveUp,
    CaretMoveDown,
    StartSelection,
    EndSelection,
    SendSelectionWord,
    MoveDialog,
    GetInputText,
    SetValueToFocusedNode,
};

struct WKImeEvent {
    WKImeEventType imeEventType;
    const char* compositionString;
    int keyCode;
    int caretIndex;
};
typedef struct WKImeEvent WKImeEvent;

WK_EXPORT WKImeEvent* WKImeEventMake(WKImeEventType imeEventType, const char* compositionString, int keyCode);
WK_EXPORT void WKImeEventRelease(WKImeEvent*);

#ifdef __cplusplus
}
#endif
