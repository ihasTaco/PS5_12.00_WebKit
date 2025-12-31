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

#include "Menu.h"
#include "MenuBar.h"
#include "URLBarText.h"
#include "WebViewWindow.h"
#include <stack>
#include <toolkitten/Widget.h>
#include <vector>

static const int MaxWebViewCount = 3;

class MainWindow : public toolkitten::Widget {
public:
    MainWindow(const char*);

    bool onKeyDown(int32_t virtualKeyCode) override;
    bool onMouseDown(toolkitten::IntPoint) override;
    bool onMouseMove(toolkitten::IntPoint) override;

    bool canCreateNewWebView() { return (m_webviewList.size() < MaxWebViewCount); }
    WebViewWindow* createNewWebView(const WebViewWindow::WebViewWindowFactory&);
    bool canCloseWebView() { return (m_webviewList.size() > 1); }
    void closeWebView(WebViewWindow*);

    void openDropdownMenu(MenuBarButton* listMenu, toolkitten::IntRect);
    void openUserAgentMenu(MenuBarButton* listMenu, toolkitten::IntRect);
    void openBookmarkMenu(MenuBarButton* listMenu, toolkitten::IntRect);
    void closeDropdownMenu();

private:
    void paintSelf(toolkitten::IntPoint) override;

    void createRootMenuItems();
    void setTitle(const char*);
    void setProgress(double); // 0.0 - 1.0
    void updateTitleBar();
    void loadURL(const char*);
    void closeMenu();

    void setActiveWebview(int index);
    WebViewWindow* activeWebView();
    void switchWebView(int delta);

    bool isOpenDropdownMenu() { return (m_dropdownMenu != nullptr); }

    void setUserAgent(const char*);
    std::string getUserAgent();

    Widget* m_titleBar = nullptr;
    MenuBar* m_rootMenu = nullptr;
    URLBarText* m_urlText = nullptr;
    MenuBarButton* m_listmenuButton = nullptr;

    Widget* m_webviewFrame = nullptr; // dummy widget

    std::vector<WebViewWindow*> m_webviewList;
    int m_activeViewIndex = -1;
    std::string m_title;
    double m_progress = 1;

    Menu* m_dropdownMenu = nullptr;
};
