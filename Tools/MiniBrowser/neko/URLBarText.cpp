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
#include "URLBarText.h"

#include <KeyboardCodes.h>

using namespace std;
using namespace toolkitten;

void URLBarText::setDecideURLCallBack(std::function<void(const char *)>&& callbackFunc)
{
    m_onDecide = std::move(callbackFunc);
}

void URLBarText::updateSelf()
{
    TextBox::updateSelf();

    if (focusedWidget() == this) {
        if (m_imeDialog) {
            if (m_imeDialog->isFinished()) {
                setText(m_imeDialog->getImeText().c_str());
                m_imeDialog.reset();
                if (m_onDecide)
                    m_onDecide(getText());
            }
        }
    }
}

bool URLBarText::onMouseMove(toolkitten::IntPoint)
{
    setFocused();
    return true;
}

bool URLBarText::onKeyDown(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_RETURN:
        setFocused();
        onSelect();
        return true;
    }
    return false;
}

void URLBarText::onSelect()
{
    EditorState editorState;
    editorState.inputType = InputFieldType::Url;
    editorState.supportLanguage = InputFieldLanguage::English_Us;
    editorState.title = "miniBrowser Input URL";
    editorState.text = string(getText());
    editorState.position = globalPosition();
    editorState.isInLoginForm = false;
    m_imeDialog = ImeDialog::create(editorState);
    if (!m_imeDialog)
        fprintf(stderr, "ERROR: ImeDialog::create()\n");
}
