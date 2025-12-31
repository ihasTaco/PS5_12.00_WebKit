/*
 * Copyright (C) 2018 Sony Computer Entertainment Inc.
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

#if WK_HAVE_C_SPI

#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <WebKit/WKContextPrivate.h>

namespace TestWebKitAPI {

struct WebKitFullScreenTest : public ::testing::Test {
    std::unique_ptr<PlatformWebView> webView;

    bool didFinishLoad { false };
    bool enterFullScreenDone { false };
    bool exitFullScreenDone { false };
    bool closeFullScreenDone { false };
    bool beganEnterFullScreenDone { false };
    bool beganExitFullScreenDone { false };

    static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.didFinishLoad = true;
    }
    static void enterFullScreen(WKViewRef view, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.enterFullScreenDone = true;
    }

    static void exitFullScreen(WKViewRef view, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.exitFullScreenDone = true;
    }

    static void closeFullScreen(WKViewRef view, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.closeFullScreenDone = true;
    }

    static void beganEnterFullScreen(WKViewRef view, const WKRect initialFrame, const WKRect finalFrame, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.beganEnterFullScreenDone = true;
    }

    static void beganExitFullScreen(WKViewRef view, const WKRect initialFrame, const WKRect finalFrame, const void* clientInfo)
    {
        WebKitFullScreenTest& client = *static_cast<WebKitFullScreenTest*>(const_cast<void*>(clientInfo));
        client.beganExitFullScreenDone = true;
    }

    static void setPageLoaderClient(WKPageRef page, const void* clientInfo)
    {
        WKPageLoaderClientV6 loaderClient;
        memset(&loaderClient, 0, sizeof(loaderClient));

        loaderClient.base.version = 6;
        loaderClient.base.clientInfo = clientInfo;
        loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

        WKPageSetPageLoaderClient(page, &loaderClient.base);
    }

    static void setViewClient(WKViewRef view, const void* clientInfo)
    {
        WKViewClientV0 viewClient;
        memset(&viewClient, 0, sizeof(viewClient));

        viewClient.base.version = 0;
        viewClient.base.clientInfo = clientInfo;
        viewClient.enterFullScreen = enterFullScreen;
        viewClient.exitFullScreen = exitFullScreen;
        viewClient.beganEnterFullScreen = beganEnterFullScreen;
        viewClient.beganExitFullScreen = beganExitFullScreen;
        viewClient.closeFullScreen = closeFullScreen;

        WKViewSetViewClient(view, &viewClient.base);
    }

    // From ::testing::Test
    void SetUp() override
    {
        WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreate());
        webView = std::make_unique<PlatformWebView>(context.get());

        // Allow fullscreen api.
        WKRetainPtr<WKPreferencesRef> preferences = adoptWK(WKPreferencesCreate());
        WKPageGroupRef pageGroup = WKPageGetPageGroup(webView->page());
        WKPreferencesSetFullScreenEnabled(preferences.get(), true);
        WKPageGroupSetPreferences(pageGroup, preferences.get());

        setViewClient(webView->platformView(), this);
        setPageLoaderClient(webView->page(), this);

        didFinishLoad = false;
        enterFullScreenDone = false;
        exitFullScreenDone = false;
        closeFullScreenDone = false;
        beganEnterFullScreenDone = false;
        beganExitFullScreenDone = false;

        WKPageLoadURL(webView->page(), adoptWK(Util::createURLForResource("playstation/fullscreen", "html")).get());
        Util::run(&didFinishLoad);
    }
};

TEST_F(WebKitFullScreenTest, ExitByAPI)
{
    webView->simulateSpacebarKeyPress();
    Util::run(&enterFullScreenDone);

    WKViewWillEnterFullScreen(webView->platformView());
    EXPECT_EQ(true, WKViewIsFullScreen(webView->platformView()));

    Util::run(&beganEnterFullScreenDone);
    WKViewDidEnterFullScreen(webView->platformView());

    WKViewRequestExitFullScreen(webView->platformView());
    Util::run(&exitFullScreenDone);

    WKViewWillExitFullScreen(webView->platformView());
    Util::run(&beganExitFullScreenDone);
    WKViewDidExitFullScreen(webView->platformView());

    EXPECT_EQ(false, WKViewIsFullScreen(webView->platformView()));
}

TEST_F(WebKitFullScreenTest, ExitByJavaScript)
{
    webView->simulateSpacebarKeyPress();
    Util::run(&enterFullScreenDone);

    WKViewWillEnterFullScreen(webView->platformView());
    EXPECT_EQ(true, WKViewIsFullScreen(webView->platformView()));

    Util::run(&beganEnterFullScreenDone);
    WKViewDidEnterFullScreen(webView->platformView());

    webView->simulateSpacebarKeyPress();
    Util::run(&exitFullScreenDone);

    WKViewWillExitFullScreen(webView->platformView());
    Util::run(&beganExitFullScreenDone);
    WKViewDidExitFullScreen(webView->platformView());

    EXPECT_EQ(false, WKViewIsFullScreen(webView->platformView()));
}

TEST_F(WebKitFullScreenTest, Close)
{
    webView->simulateSpacebarKeyPress();
    Util::run(&enterFullScreenDone);

    WKViewWillEnterFullScreen(webView->platformView());
    EXPECT_EQ(true, WKViewIsFullScreen(webView->platformView()));

    WKPageClose(webView->page());
    Util::run(&closeFullScreenDone);
    EXPECT_EQ(false, WKViewIsFullScreen(webView->platformView()));
}
} // namespace TestWebKitAPI

#endif
