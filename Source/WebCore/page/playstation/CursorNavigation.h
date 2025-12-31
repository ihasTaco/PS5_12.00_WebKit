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

#if ENABLE(CURSOR_NAVIGATION)

#include "FocusDirection.h"
#include "IntPoint.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"

namespace WebCore {

class LocalFrame;
class Page;

enum class CursorNavDirection: uint8_t {
    Up = static_cast<uint8_t>(FocusDirection::Up),
    Down = static_cast<uint8_t>(FocusDirection::Down),
    Left = static_cast<uint8_t>(FocusDirection::Left),
    Right = static_cast<uint8_t>(FocusDirection::Right),
};

class CursorNavigation {
WTF_MAKE_FAST_ALLOCATED;

public:
    CursorNavigation(Page&);
    bool moveCursor(CursorNavDirection, bool& moved, IntPoint& newCursorPosition);
    bool handleKeyEvent(const PlatformKeyboardEvent&, bool& moved, IntPoint& newCursorPosition);
    void setCursorPosition(const PlatformMouseEvent&);

private:
    Page& m_page;
    IntPoint m_cursorPosition;
};

bool isCursorNavigationEnabled(const LocalFrame*);

} // namespace WebCore
#endif
