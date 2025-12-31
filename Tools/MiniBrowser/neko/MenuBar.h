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

#pragma once

#include "Menu.h"
#include "WebViewWindow.h"

#include <KeyboardCodes.h>
#include <toolkitten/Button.h>
#include <toolkitten/ImageButton.h>

class MenuBar : public toolkitten::Widget {
public:
    MenuBar();
    MenuBar(unsigned);

    void closeMenu();

    Menu* addSubListMenu(std::unique_ptr<Menu>&&);

    void setFocused() override;
    bool onKeyDown(int32_t) override;
    void updateSelf() override;
    void paintSelf(toolkitten::IntPoint) override { };

    static const int ItemWidth = 180;

private:
    uint32_t m_focusedItemIndex = 0;
    Menu* m_subListMenu = nullptr;
};

class MenuBarButton : public toolkitten::ImageButton {
public:
    using OnDecideCallback = std::function<void(MenuBarButton*, toolkitten::IntRect)>;

    MenuBarButton(MenuBar* menu)
        : m_menubar(menu)
    {
    }

    MenuBar* menuBar() { return m_menubar; }

    void setOnDecide(OnDecideCallback&& callback)
    {
        m_onDecide = std::move(callback);
    }

protected:
    bool onMouseMove(toolkitten::IntPoint) override
    {
        setFocused();
        return true;
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
        m_menubar->closeMenu(); // Close the sublistmenu
        if (m_onDecide)
            m_onDecide(this, toolkitten::IntRect { globalPosition(), size() });
    }

    OnDecideCallback m_onDecide;
    MenuBar* m_menubar = nullptr;
};
