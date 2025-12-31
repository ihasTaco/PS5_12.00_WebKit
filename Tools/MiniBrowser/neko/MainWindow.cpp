/*
 * Copyright (C) 2020 Sony Interactive Entertainment Inc.
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
#include "MainWindow.h"

#include "Bookmark.h"
#include "ImageButton.h"
#include "UserAgent.h"
#include <KeyboardCodes.h>
#include <WebKit/WKString.h>
#include <toolkitten/Application.h>

using namespace std;
using namespace toolkitten;

const int LineHeight = 40;
const int FontSize = 32;

// A simple text file, where Initial URL to load.
const char* s_initialURLFilePath = "/hostapp/MiniBrowserInitialURL.txt";

MainWindow::MainWindow(const char* requestedURL)
{
    IntSize size = Application::singleton().windowSize();
    setSize(size);

    const unsigned titleBarHeight = LineHeight * 1;
    const unsigned toolBarHeight  = LineHeight * 1;

    // regist widgets <DO NOT CHANGE The Order !>
    auto webviewFrame = make_unique<Widget>();
    IntPoint position = { 0, titleBarHeight + toolBarHeight };
    webviewFrame->setPosition(position);
    webviewFrame->setSize({ size.w, size.h - position.y });
    m_webviewFrame = webviewFrame.get();
    appendChild(std::move(webviewFrame));
    // Create WebView
    createNewWebView(WebViewWindow::PlainWebViewWindowFactory);

    // Create rootMenu
    auto menuBar = make_unique<MenuBar>(size.w);
    m_rootMenu = menuBar.get();
    m_rootMenu->setPosition({ 0, titleBarHeight });
    createRootMenuItems();
    appendChild(std::move(menuBar));

    // Create TitleBar
    auto titleBar = make_unique<Widget>();
    m_titleBar = titleBar.get();
    m_titleBar->setSize({ size.w, titleBarHeight });
    m_titleBar->fill(WHITE);
    appendChild(std::move(titleBar));
    updateTitleBar();

    // Create CloseButton
    const int closeButtonMargin = 4;
    auto closeButton = make_unique<::ImageButton>();
    closeButton->setSize({ titleBarHeight - closeButtonMargin * 2, titleBarHeight - closeButtonMargin  * 2 });
    closeButton->setPosition({ (int)(size.w - titleBarHeight + closeButtonMargin), closeButtonMargin });
    closeButton->setImageFromResource("icon_cross.png");
    closeButton->setOnDecide([](::ImageButton*) {
        exit(0);
    });
    appendChild(std::move(closeButton));

    if (requestedURL) {
        loadURL(requestedURL);
        return;
    }

    // load initial URL from file, if one was not already requested via the command line
    std::string initialURL = "http://www.google.com/";
    FILE* fin = fopen(s_initialURLFilePath, "r");
    if (fin) {
        const int BufferSize = 4096;
        char buffer[BufferSize];
        char* b = buffer;
        if (fgets(buffer, BufferSize, fin)) {
            while (0x7F < static_cast<unsigned>(*b))
                b++;
            initialURL = b;
        }
        fclose(fin);
    } else {
        // Create a new initial url file.
        FILE* fout = fopen(s_initialURLFilePath, "w");
        if (fout) {
            fputs("\xEF\xBB\xBF", fout); // UTF-8 BOM
            fputs(initialURL.c_str(), fout);
            fclose(fout);
        }
    }
    loadURL(initialURL.c_str());
}

bool MainWindow::onKeyDown(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_L1:
        activeWebView()->goBack();
        return true;
    case VK_R1:
        activeWebView()->goForward();
        return true;
    case VK_R2:
        switchWebView(1);
        return true;
    case VK_L2:
        switchWebView(-1);
        return true;
    case VK_R3:
        activeWebView()->toggleZoomFactor();
        return true;
    case VK_L3:
        activeWebView()->toggleViewScaleFactor();
        return true;
    case VK_TRIANGLE:
        if (isOpenDropdownMenu()) {
            closeDropdownMenu();
            setFocused();
        } else {
            m_listmenuButton->setFocused();
            openDropdownMenu(m_listmenuButton, toolkitten::IntRect { m_listmenuButton->position(), m_listmenuButton->size() });
        }
        return true;
    case VK_DOWN:
    case VK_RIGHT:
        if (isOpenDropdownMenu()) {
            m_dropdownMenu->setFocused();
            return true;
        }
    }
    return false;
}

WebViewWindow* MainWindow::createNewWebView(const WebViewWindow::WebViewWindowFactory& factory)
{
    if (!canCreateNewWebView())
        return nullptr;
    IntSize size = m_webviewFrame->size();
    IntPoint position = { 0, 0};

    auto webViewWindow = factory();
    webViewWindow->setUserData(this);
    webViewWindow->setPosition(position);
    webViewWindow->setSize({ size.w, size.h });
    webViewWindow->fill(WHITE);
    webViewWindow->setOnSetTitleCallBack([this](const char* title) {
        setTitle(title);
    });
    webViewWindow->setOnSetURLCallBack([this](const char* url) {
        if (m_urlText)
            m_urlText->setText(url, FontSize);
    });
    webViewWindow->setOnSetProgressCallBack([this](double progress) {
        setProgress(progress);
    });
    webViewWindow->setOnCreateNewViewCallback([this](const WebViewWindow::WebViewWindowFactory& factory) {
        WebViewWindow* newView = nullptr;
        if (canCreateNewWebView())
            newView = createNewWebView(factory);
        return newView;
    });
    webViewWindow->setOnRequestCloseViewCallback([this](WebViewWindow* view) {
        if (canCloseWebView())
            closeWebView(view);
    });

    WebViewWindow* rawWebViewWindow = webViewWindow.get();

    m_webviewList.push_back(rawWebViewWindow);
    m_webviewFrame->appendChild(std::move(webViewWindow));
    setActiveWebview(m_webviewList.size() - 1);

    return rawWebViewWindow;
}

void MainWindow::closeWebView(WebViewWindow* view)
{
    std::vector<WebViewWindow*>::iterator i;
    for (i = m_webviewList.begin(); i != m_webviewList.end(); ++i) {
        if (*i == view) {
            m_webviewFrame->removeChild(view);
            m_webviewList.erase(i);
            switchWebView(0);
            break;
        }
    }
}

WebViewWindow* MainWindow::activeWebView()
{
    return (m_activeViewIndex != -1 && m_activeViewIndex < MaxWebViewCount) ?  m_webviewList[m_activeViewIndex] : nullptr;
}

void MainWindow::openDropdownMenu(MenuBarButton* listMenu, toolkitten::IntRect)
{
    auto menu = make_unique<Menu>();
    menu->setUserData(this);

    if (canCloseWebView()) {
        menu->addItem("Close Window", [&](Menu* menu, IntRect) {
            if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
                mainWindow->closeWebView(mainWindow->activeWebView());
                mainWindow->closeDropdownMenu();
            }
        });
    }

    menu->addItem("Create New Window", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            mainWindow->createNewWebView(WebViewWindow::PlainWebViewWindowFactory);
            mainWindow->closeDropdownMenu();
        }
    });

    menu->addItem("Quit", [&](Menu*, IntRect) {
        exit(0);
    });

    menu->addItem("User Agent", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            mainWindow->closeDropdownMenu();
            mainWindow->openUserAgentMenu(m_listmenuButton, toolkitten::IntRect { m_listmenuButton->position(), m_listmenuButton->size() });
        }
    });

    menu->addItem("Bookmarks", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            mainWindow->closeDropdownMenu();
            mainWindow->openBookmarkMenu(m_listmenuButton, toolkitten::IntRect { m_listmenuButton->position(), m_listmenuButton->size() });
        }
    });

    menu->addItem("Delete cookie", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->deleteCookie();
            mainWindow->closeDropdownMenu();
        }
    });

    menu->addItem("Delete website data", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->deleteWebsiteData();
            mainWindow->closeDropdownMenu();
        }
    });

    menu->addItem("Remove disk cache", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->removeNetworkCache();
            mainWindow->closeDropdownMenu();
        }
    });

    menu->addItem("About MiniBrowser", [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            loadURL("about:");
            mainWindow->closeDropdownMenu();
        }
    });

    auto acceleratedCompositingString = activeWebView()->getAcceleratedCompositing() ? "[x] Accelerated Compositing" : "[  ] Accelerated Compositing";
    menu->addItem(acceleratedCompositingString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setAcceleratedCompositing(!activeWebView()->getAcceleratedCompositing());
            mainWindow->closeDropdownMenu();
        }
    });

    auto compositingBordersString = activeWebView()->getCompositingBordersVisible() ? "[x] CompositingBorders" : "[  ] CompositingBorders";
    menu->addItem(compositingBordersString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setCompositingBordersVisible(!activeWebView()->getCompositingBordersVisible());
            mainWindow->closeDropdownMenu();
        }
    });

    auto tiledScrollingIndicatorString = activeWebView()->getTiledScrollingIndicatorVisible() ? "[x] TiledScrollingIndicator" : "[  ] TiledScrollingIndicator";
    menu->addItem(tiledScrollingIndicatorString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setTiledScrollingIndicatorVisible(!activeWebView()->getTiledScrollingIndicatorVisible());
            mainWindow->closeDropdownMenu();
        }
    });

    auto drawsBackgroundString = activeWebView()->getDrawsBackground() ? "[x] DrawsBackground" : "[  ] DrawsBackground";
    menu->addItem(drawsBackgroundString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setDrawsBackground(!activeWebView()->getDrawsBackground());
            mainWindow->closeDropdownMenu();
        }
    });

    auto largeImageAsyncDecodingString = activeWebView()->getLargeImageAsyncDecodingEnabled() ? "[x] LargeImageAsyncDecoding" : "[  ] LargeImageAsyncDecoding";
    menu->addItem(largeImageAsyncDecodingString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setLargeImageAsyncDecodingEnabled(!activeWebView()->getLargeImageAsyncDecodingEnabled());
            mainWindow->closeDropdownMenu();
        }
    });

    auto videoPlaybackRequiresUserGestureString = activeWebView()->getVideoPlaybackRequiresUserGesture() ? "[  ] Media autoplay" : "[x] Media autoplay";
    menu->addItem(videoPlaybackRequiresUserGestureString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setVideoPlaybackRequiresUserGesture(!activeWebView()->getVideoPlaybackRequiresUserGesture());
            mainWindow->closeDropdownMenu();
        }
    });

    auto cookieString = activeWebView()->getCookieEnabled() ? "[x] Cookie" : "[  ] Cookie";
    menu->addItem(cookieString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->toggleCookieEnabled();
            mainWindow->closeDropdownMenu();
        }
    });

    auto resourceLoadStatisticsString = activeWebView()->getResourceLoadStatisticsEnabled() ? "[x] ResourceLoadStatistics" : "[  ] ResourceLoadStatistics";
    menu->addItem(resourceLoadStatisticsString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setResourceLoadStatisticsEnabled(!activeWebView()->getResourceLoadStatisticsEnabled());
            mainWindow->closeDropdownMenu();
        }
    });

#if ENABLE(ACCESSIBILITY)
    auto accessibilityString = activeWebView()->getAccessibilityEnabled() ? "[x] Accessibility" : "[  ] Accessibility";
    menu->addItem(accessibilityString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setAccessibilityEnabled(!activeWebView()->getAccessibilityEnabled());
            mainWindow->closeDropdownMenu();
        }
    });
#endif

    auto reloadWhenWebProcessDidTerminateString = activeWebView()->getReloadWhenWebProcessDidTerminate() ? "[x] Reload if process crashed" : "[  ] Reload if process crashed";
    menu->addItem(reloadWhenWebProcessDidTerminateString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->setReloadWhenWebProcessDidTerminate(!activeWebView()->getReloadWhenWebProcessDidTerminate());
            mainWindow->closeDropdownMenu();
        }
    });

#if PLATFORM(PLAYSTATION) // for downstream-only
    auto networkBandwidthString = (activeWebView()->getNetworkBandWidthMode() == kWKNetworkBandwidthModePrivileged) ? "[x] NetworkBandwidth Privileged" : "[  ] NetworkBandwidth Privileged";
    menu->addItem(networkBandwidthString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->toggleNetworkBandWidthMode();
            mainWindow->closeDropdownMenu();
        }
    });

    auto kidsSafetyString = activeWebView()->isWebSecurityTMServiceEnabled(WebViewWindow::WebSecurityTMService::kKidsSafety) ? "[x] WebSecurity(Kids Safety)" : "[  ] WebSecurity(Kids Safety)";
    menu->addItem(kidsSafetyString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->toggleWebSecurityTMService(WebViewWindow::WebSecurityTMService::kKidsSafety);
            mainWindow->closeDropdownMenu();
        }
    });

    auto antiPhishingString = activeWebView()->isWebSecurityTMServiceEnabled(WebViewWindow::WebSecurityTMService::kAntiPhishing) ? "[x] WebSecurity(Anti Phishing)" : "[  ] WebSecurity(Anti Phishing)";
    menu->addItem(antiPhishingString, [&](Menu* menu, IntRect) {
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
            activeWebView()->toggleWebSecurityTMService(WebViewWindow::WebSecurityTMService::kAntiPhishing);
            mainWindow->closeDropdownMenu();
        }
    });
#endif

    m_dropdownMenu = listMenu->menuBar()->addSubListMenu(std::move(menu));
}

void MainWindow::openUserAgentMenu(MenuBarButton* listMenu, toolkitten::IntRect)
{
    auto menu = make_unique<Menu>();
    menu->setUserData(this);

    std::string currentUserAgent = static_cast<MainWindow*>(menu->userData())->getUserAgent();

    listUserAgent(currentUserAgent, [&](UserAgent&& userAgent) {
        menu->addItem(userAgent.title.c_str(), [uaString = userAgent.uaString](Menu* menu, IntRect) {
            if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
                mainWindow->setUserAgent(uaString.c_str());
                mainWindow->closeDropdownMenu();
            }
        });
        return true;
    });

    m_dropdownMenu = listMenu->menuBar()->addSubListMenu(std::move(menu));
}

void MainWindow::openBookmarkMenu(MenuBarButton* listMenu, toolkitten::IntRect)
{
    auto menu = make_unique<Menu>();
    menu->setUserData(this);

    menu->addItem("Add Bookmark", [&](Menu* menu, IntRect) {
        addBookmark({ m_title, m_urlText->getText() });
        if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData()))
            mainWindow->closeDropdownMenu();
    });

    listBookmark([&](Bookmark&& bookmark) {
        menu->addItem(bookmark.title.c_str(), [url = bookmark.url](Menu* menu, IntRect) {
            if (MainWindow* mainWindow = static_cast<MainWindow*>(menu->userData())) {
                mainWindow->loadURL(url.c_str());
                mainWindow->closeDropdownMenu();
            }
        });
        return true;
    });

    m_dropdownMenu = listMenu->menuBar()->addSubListMenu(std::move(menu));
}

void MainWindow::closeDropdownMenu()
{
    if (m_rootMenu)
        m_rootMenu->closeMenu();
    m_dropdownMenu = nullptr;
}

bool MainWindow::onMouseDown(toolkitten::IntPoint)
{
    closeDropdownMenu();
    return true;
}

bool MainWindow::onMouseMove(toolkitten::IntPoint point)
{
    IntRect titleBarRect = { {0, 0}, {m_size.w, LineHeight} };

    if (titleBarRect.hits(point)) {
        setFocused();
        return true;
    }
    return false;
}

void MainWindow::paintSelf(toolkitten::IntPoint)
{
}

void MainWindow::createRootMenuItems()
{
    const unsigned windowWidth = size().w;
    const unsigned buttonWidth = 100;
    const unsigned urlBarWidth = windowWidth - MenuBar::ItemWidth - buttonWidth * 3;
    int x = 0;

    auto button = make_unique<MenuBarButton>(m_rootMenu);
    button->setSize({ buttonWidth, LineHeight });
    button->setPosition({ x, 0 });
    button->setImageFromResource("icon_back.png");
    button->setOnDecide([this](MenuBarButton*, toolkitten::IntRect) {
        if (auto webView = activeWebView())
            webView->goBack();
    });
    m_rootMenu->appendChild(std::move(button));
    x += buttonWidth;

    button = make_unique<MenuBarButton>(m_rootMenu);
    button->setSize({ buttonWidth, LineHeight });
    button->setPosition({ x, 0 });
    button->setImageFromResource("icon_forward.png");
    button->setOnDecide([this](MenuBarButton*, toolkitten::IntRect) {
        if (auto webView = activeWebView())
            webView->goForward();
    });
    m_rootMenu->appendChild(std::move(button));
    x += buttonWidth;

    button = make_unique<MenuBarButton>(m_rootMenu);
    button->setSize({ buttonWidth, LineHeight });
    button->setPosition({ x, 0 });
    button->setImageFromResource("icon_reload.png");
    button->setOnDecide([this](MenuBarButton*, toolkitten::IntRect) {
        if (auto webView = activeWebView())
            webView->reload();
    });
    m_rootMenu->appendChild(std::move(button));
    x += buttonWidth;

    auto urlBar = make_unique<URLBarText>();
    m_urlText = urlBar.get();
    m_urlText->setDecideURLCallBack([this](const char *url) {
        loadURL(url);
    });
    m_urlText->setName("URLBar");
    m_urlText->setSize({ urlBarWidth, LineHeight });
    m_urlText->setPosition({ x, 0 });
    m_urlText->setText("address bar", FontSize);
    m_rootMenu->appendChild(std::move(urlBar));
    x += urlBarWidth;

    button = make_unique<MenuBarButton>(m_rootMenu);
    button->setSize({ MenuBar::ItemWidth, LineHeight });
    button->setPosition({ (int)(windowWidth - MenuBar::ItemWidth), 0 });
    button->setImageFromResource("icon_menu.png");
    button->setOnDecide([this](MenuBarButton* menu, toolkitten::IntRect rect) {
        if (isOpenDropdownMenu())
            closeDropdownMenu();
        else
            openDropdownMenu(menu, rect);
    });
    m_listmenuButton = button.get();
    m_rootMenu->appendChild(std::move(button));
}

void MainWindow::setTitle(const char* title)
{
    if (!strcmp(m_title.c_str(), title))
        return;

    m_title = title;
    updateTitleBar();
}

void MainWindow::setProgress(double progress)
{
    progress = max<double>(0, progress);
    progress = min<double>(progress, 100);
    if (m_progress == progress)
        return;

    m_progress = progress;
    updateTitleBar();
}

void MainWindow::updateTitleBar()
{
    if (!m_titleBar)
        return;

    std::string titleStr;
    if (m_webviewList.size() > 1) {
        char pageIndex[8];
        snprintf(pageIndex, sizeof(pageIndex), "[%d/%lu]", m_activeViewIndex + 1, m_webviewList.size());
        titleStr = pageIndex;
        titleStr += m_title;
    } else
        titleStr = m_title;

    IntPoint pos = {0, 0};
    IntSize size = {m_size.w, LineHeight};
    int progressWidth = m_size.w * m_progress;

    IntRect rect {pos, size};
    IntRect progressRect = rect;
    progressRect.size.w = progressWidth;
    if (!progressRect.isEmpty())
        m_titleBar->fillRect(progressRect, Color {0.0, 0.0, 0.5, 1.0});

    IntRect remainingRect = rect;
    remainingRect.size.w -= progressWidth;
    remainingRect.position.x += progressWidth;
    if (!remainingRect.isEmpty())
        m_titleBar->fillRect(remainingRect, Color {0.2, 0.2, 0.7, 1.0});

    m_titleBar->strokeRect(rect, 4, Color {0.7, 0.7, 0.7, 1.0});

    pos.x += 2;
    m_titleBar->drawText(titleStr.c_str(), pos, FontSize, WHITE);
}

void MainWindow::loadURL(const char* url)
{
    m_urlText->setText(url, FontSize);
    activeWebView()->loadURL(url);
}

void MainWindow::closeMenu()
{
    if (m_rootMenu)
        removeChild(m_rootMenu);

    m_rootMenu = nullptr;

    setFocused();
}

void MainWindow::setActiveWebview(int activeIndex)
{
    // Make current web view hidden
    if (m_activeViewIndex != -1 && static_cast<size_t>(m_activeViewIndex) < m_webviewList.size())
        activeWebView()->setActive(false);

    m_activeViewIndex = activeIndex;
    activeWebView()->setActive(true);
}

void MainWindow::switchWebView(int delta)
{
    if (isOpenDropdownMenu())
        closeDropdownMenu();

    unsigned size = m_webviewList.size();
    setActiveWebview((m_activeViewIndex + size + delta) % size);
}

void MainWindow::setUserAgent(const char* userAgent)
{
    activeWebView()->setUserAgent(userAgent);
}

std::string MainWindow::getUserAgent()
{
    return activeWebView()->getUserAgent();
}
