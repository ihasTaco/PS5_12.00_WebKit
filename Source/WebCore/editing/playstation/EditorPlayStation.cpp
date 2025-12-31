/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Editor.h"

#include "CompositionEvent.h"
#include "Document.h"
#include "Element.h"
#include "EventNames.h"
#include "LocalFrame.h"
#include "NotImplemented.h"
#include "Range.h"
#include "Text.h"
#include "TypingCommand.h"
#include "UserTypingGestureIndicator.h"

namespace WebCore {

void Editor::writeSelectionToPasteboard(Pasteboard&)
{
    notImplemented();
}

void Editor::writeImageToPasteboard(Pasteboard&, Element&, const URL&, const String&)
{
    notImplemented();
}

void Editor::pasteWithPasteboard(Pasteboard*, OptionSet<PasteOption>)
{
    notImplemented();
}

void Editor::confirmComposition(const String& text, unsigned caretIndexInText)
{
    confirmComposition(text);

    Position base = m_document.selection().selection().base().downstream();
    Position extent = m_document.selection().selection().extent();
    Node* baseNode = base.deprecatedNode();
    unsigned baseOffset = base.deprecatedEditingOffset();
    Node* extentNode = extent.deprecatedNode();

    if (is<Text>(baseNode) && baseNode == extentNode) {
        unsigned actualCaretIndex = (baseOffset - text.length() + caretIndexInText);
        m_document.selection().setSelectedRange(SimpleRange { { *baseNode, actualCaretIndex }, { *baseNode, actualCaretIndex } }, Affinity::Downstream, FrameSelection::ShouldCloseTyping::No);
    }
}

void Editor::platformCopyFont()
{
}

void Editor::platformPasteFont()
{
}

} // namespace WebCore
