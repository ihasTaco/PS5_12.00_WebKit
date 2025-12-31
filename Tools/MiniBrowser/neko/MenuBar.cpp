/*
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
#include "MenuBar.h"

#include <toolkitten/Cursor.h>

using namespace std;
using namespace toolkitten;

MenuBar::MenuBar()
{
    setSize({ ItemWidth, 0 });
}

MenuBar::MenuBar(unsigned width)
{
    setSize({ width, 0 });
}

void MenuBar::closeMenu()
{
    // Close the existing submenu.
    if (m_subListMenu)
        removeChild(m_subListMenu);
    m_subListMenu = nullptr;
}


Menu* MenuBar::addSubListMenu(std::unique_ptr<Menu>&& menu)
{
    // Close the existing submenu.
    closeMenu();

    if (!menu)
        return nullptr;

    m_subListMenu = menu.get();
    m_subListMenu->setPosition({ (int)(size().w - menu->size().w), 0 });
    appendChild(std::move(menu));
    return m_subListMenu;
}

void MenuBar::setFocused()
{
    auto it = m_children.begin();
    advance(it, m_focusedItemIndex);
    if (it == m_children.end())
        return;

    (*it)->setFocused();
}

void MenuBar::updateSelf()
{
    // Update focused item index
    uint32_t i = 0;
    for (auto& it : m_children) {
        if (it.get() == focusedWidget()) {
            m_focusedItemIndex = i;
            break;
        }
        i++;
    }
}

bool MenuBar::onKeyDown(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_RIGHT:
        // Focus the previous item.
        if (m_focusedItemIndex + 1 < m_children.size()) {
            m_focusedItemIndex++;
            setFocused();
        }
        return true;
    case VK_LEFT:
        // Focus the next item.
        if (m_focusedItemIndex > 0) {
            m_focusedItemIndex--;
            setFocused();
        }
        return true;
    case VK_ESCAPE:
        // Close the submenu.
        if (m_subListMenu) {
            addSubListMenu(nullptr);
            setFocused();
            return true;
        }
        break;
    }
    return false;
}
