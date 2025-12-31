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
#include "Menu.h"

#include <KeyboardCodes.h>
#include <toolkitten/Button.h>
#include <toolkitten/Cursor.h>

using namespace std;
using namespace toolkitten;

const int ItemHeight = 40;
const int ItemWidth = 500;
const int FontSize = 32;

class MenuItem : public Button {
public:
    std::function<void(Menu*, IntRect)> m_onDecide;

    MenuItem(Menu* menu)
        : m_menu(menu)
    {
    }

protected:
    bool onMouseDown(IntPoint) override
    {
        m_captureMouseDown = true;
        return true;
    }

    bool onMouseUp(IntPoint) override
    {
        if (m_captureMouseDown)
            onDecide();
        m_captureMouseDown = false;
        return true;
    }

    bool onMouseMove(IntPoint) override
    {
        setFocused();
        return true;
    }

    bool onKeyDown(int32_t virtualKeyCode) override
    {
        switch (virtualKeyCode) {
        case VK_RETURN:
        case VK_RIGHT:
            return true;
        }
        return false;
    }

    bool onKeyUp(int32_t virtualKeyCode) override
    {
        switch (virtualKeyCode) {
        case VK_RETURN:
        case VK_RIGHT:
            onDecide();
            return true;
        }
        return false;
    }

    void onDecide() override
    {
        if (m_onDecide)
            m_onDecide(m_menu, IntRect { globalPosition(), size() });
    }

    Menu* m_menu;
    bool m_captureMouseDown { false };
};

Menu::Menu()
{
    setSize({ ItemWidth, 0 });
}

Widget* Menu::addItem(const char* label, std::function<void()>&& onDecide)
{
    return addItem(label, [onDecide = std::move(onDecide)](Menu*, IntRect) {
        onDecide();
    });
}

Widget* Menu::addItem(const char* label, std::function<void(Menu*, IntRect)>&& onDecide)
{
    int numItems = m_children.size();

    auto item = make_unique<MenuItem>(this);
    item->m_onDecide = std::move(onDecide);
    item->setSize({ ItemWidth, ItemHeight });
    item->setPosition({ 0 , ItemHeight * (numItems + 1) });
    item->setText(label, FontSize);
    Widget* result = item.get();

    appendChild(std::move(item));

    return result;
}

Menu* Menu::addSubMenu(std::unique_ptr<Menu>&& menu)
{
    // Close the existing submenu.
    if (m_subMenu)
        removeChild(m_subMenu);

    m_subMenu = nullptr;

    if (!menu)
        return nullptr;

    m_subMenu = menu.get();
    m_subMenu->setPosition({ (int)size().w, (int)(ItemHeight * m_focusedItemIndex) });
    appendChild(std::move(menu));
    m_subMenu->setFocused();
    return m_subMenu;
}

void Menu::setFocused()
{
    auto it = m_children.begin();
    advance(it, m_focusedItemIndex);
    if (it == m_children.end())
        return;

    (*it)->setFocused();
}

void Menu::updateSelf()
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

bool Menu::onKeyDown(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_DOWN:
        // Focus the next item.
        m_focusedItemIndex = (++m_focusedItemIndex % m_children.size());
        setFocused();
        return true;
    case VK_UP:
        // Focus the previous item.
        m_focusedItemIndex = ((m_focusedItemIndex + m_children.size() -1) % m_children.size());
        setFocused();
        return true;
    case VK_ESCAPE:
    case VK_LEFT:
        // Close the submenu.
        if (m_subMenu) {
            addSubMenu(nullptr);
            setFocused();
            return true;
        }
        break;
    }
    return false;
}
