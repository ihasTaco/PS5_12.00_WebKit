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

#include "FocusWidget.h"
#include "Menu.h"
#include "OpenGLServerProcessProxy.h"
#include "PunchHoleWidget.h"
#include <WebKit/WKContext.h>
#include <WebKit/WKContextPlayStation.h>
#include <WebKit/WKCookieManager.h>
#include <WebKit/WKImeEvent.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKView.h>
#include <WebKit/WKViewAccessibilityClient.h>
#include <WebKit/WKWebsiteDataStoreRefPlayStation.h>
#include <chrono>
#include <memory>
#include <optional>
#include <toolkitten/ImeDialog.h>
#include <toolkitten/Widget.h>
#include <unordered_map>

class WebViewWindow : public toolkitten::Widget {
public:
    enum {
        kNotifyURL = 1 << 0,
        kNotifyTitle = 1 << 1,
        kNotifyProgress = 1 << 2,
        kNotifyALL = kNotifyURL | kNotifyTitle | kNotifyProgress
    };
    typedef uint32_t NotifyInformations;

#if PLATFORM(PLAYSTATION) // for downstream-only
    enum WebSecurityTMService {
        kKidsSafety = 1 << 0,
        kAntiPhishing = 1 << 1
    };
#endif

    static std::unique_ptr<WebViewWindow> create();

    explicit WebViewWindow(WKPageConfigurationRef);
    virtual ~WebViewWindow();

    void loadURL(const char* url);
    void goBack();
    void goForward();
    void reload();
    void setActive(bool);
    void setUserAgent(const char*);
    std::string getUserAgent();

    void setOnSetTitleCallBack(std::function<void(const char*)>&& onSetTitle_cb);
    void setOnSetURLCallBack(std::function<void(const char*)>&& onSetURL_cb);
    void setOnSetProgressCallBack(std::function<void(double)>&& onSetProgress_cb);

    using WebViewWindowFactory = std::function<std::unique_ptr<WebViewWindow>()>;
    using OnCreateNewViewCallback = std::function<WebViewWindow*(const WebViewWindowFactory&)>;
    static const WebViewWindowFactory PlainWebViewWindowFactory;
    void setOnCreateNewViewCallback(OnCreateNewViewCallback&& onCreateNewView_cb);

    void setOnRequestCloseViewCallback(std::function<void(WebViewWindow*)>&& onRequestCloseView_cb);

    void setSize(toolkitten::IntSize) override;
    bool onKeyUp(int32_t virtualKeyCode) override;
    bool onKeyDown(int32_t virtualKeyCode) override;
    bool onMouseMove(toolkitten::IntPoint) override;
    bool onMouseUp(toolkitten::IntPoint) override;
    bool onMouseDown(toolkitten::IntPoint) override;
    bool onWheelMove(toolkitten::IntPoint, toolkitten::IntPoint) override;

    // For WKViewClient
    static void viewNeedsDisplay(WKViewRef, WKRect, const void*);
    static void enterFullScreen(WKViewRef, const void*);
    static void exitFullScreen(WKViewRef, const void*);
    static void doneWithMouseUpEvent(WKViewRef, bool, const void*);
    static void didChangeEditorState(WKViewRef, WKEditorState, WKRect, WKStringRef, const void*);
    static void setCursorPosition(WKViewRef, WKPoint cursorPosition, const void* clientInfo);
    static void setCursor(WKViewRef, WKCursorType, const void* clientInfo);
    static void enterAcceleratedCompositingMode(WKViewRef, uint32_t canvasHandle, const void*);
    static void exitAcceleratedCompositingMode(WKViewRef, const void*);
    static void didChangeContentsVisibility(WKViewRef, WKSize, WKPoint, float scale, const void* clientInfo);
    static int launchOpenGLServerProcess(WKViewRef, const void* clientInfo);

    // For WKPageNavigationClient
    static void decidePolicyForNavigationAction(WKPageRef, WKNavigationActionRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    static void decidePolicyForNavigationResponse(WKPageRef, WKNavigationResponseRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    static void didReceiveAuthenticationChallenge(WKPageRef, WKAuthenticationChallengeRef, const void*);
    static void webProcessDidTerminate(WKPageRef, WKProcessTerminationReason, const void*);

    // For WKPageUIClient
    static WKPageRef createNewPage(WKPageRef, WKPageConfigurationRef, WKNavigationActionRef, WKWindowFeaturesRef, const void* clientInfo);
    static void runJavaScriptAlert(WKPageRef, WKStringRef, WKFrameRef, WKSecurityOriginRef, WKPageRunJavaScriptAlertResultListenerRef, const void* clientInfo);
    static void close(WKPageRef, const void* clientInfo);
    static void decidePolicyForMediaKeySystemPermissionRequest(WKPageRef, WKSecurityOriginRef, WKStringRef, WKMediaKeySystemPermissionCallbackRef);
    static void didNotHandleWheelEvent(WKPageRef, WKNativeEventPtr, const void* clientInfo);
    static WKRect getWindowFrame(WKPageRef, const void* clientInfo);

    // For WKPageStateClient
    static void didChangeTitle(const void* clientInfo);
    static void didChangeActiveURL(const void* clientInfo);
    static void didChangeEstimatedProgress(const void* clientInfo);
    static void didChangeWebProcessIsResponsive(const void* clientInfo);
    static void didSwapWebProcesses(const void* clientInfo);

    bool getCookieEnabled();
    void toggleCookieEnabled();
    void updateCookieAcceptPolicy(WKHTTPCookieAcceptPolicy);

#if PLATFORM(PLAYSTATION) // for downstream-only
    WKNetworkBandwidthMode getNetworkBandWidthMode() const { return m_networkBandwidthMode; }
    void toggleNetworkBandWidthMode();

    bool isWebSecurityTMServiceEnabled(WebSecurityTMService);
    void toggleWebSecurityTMService(WebSecurityTMService);
#endif

    void toggleScaleFactor();
    void toggleZoomFactor();
    void toggleViewScaleFactor();

    // unshared settings (only active view)
    void setDrawsBackground(bool);
    bool getDrawsBackground() { return m_drawsBackground; }
    void setAcceleratedCompositing(bool);
    bool getAcceleratedCompositing() { return m_acceleratedCompositingEnabled; }
    void setReloadWhenWebProcessDidTerminate(bool);
    bool getReloadWhenWebProcessDidTerminate() { return m_reloadWhenWebProcessDidTerminate; }

    // shared settings (for all view)
    void deleteCookie();
    void deleteWebsiteData();
    void setResourceLoadStatisticsEnabled(bool);
    bool getResourceLoadStatisticsEnabled();
    void setCompositingBordersVisible(bool);
    bool getCompositingBordersVisible();
    void setTiledScrollingIndicatorVisible(bool);
    bool getTiledScrollingIndicatorVisible();
    void setLargeImageAsyncDecodingEnabled(bool);
    bool getLargeImageAsyncDecodingEnabled();
    void setVideoPlaybackRequiresUserGesture(bool);
    bool getVideoPlaybackRequiresUserGesture();
    void setAccessibilityEnabled(bool);
    bool getAccessibilityEnabled();
    void removeNetworkCache();

private:
    class WebContext {
    protected:
        WebContext();

    public:
        ~WebContext();
        static std::shared_ptr<WebContext> singleton()
        {
            auto ret = s_instance.lock();
            if (!ret) {
                ret = std::shared_ptr<WebContext>(new WebContext());
                s_instance = ret;
            }
            return s_instance.lock();
        }

        WKContextRef context() { return m_context.get(); }
        WKPageGroupRef pageGroup() { return m_pageGroup.get(); }
        WKPreferencesRef preferences() { return m_preferencesMaster.get(); }
        WKWebsiteDataStoreRef websiteDataStore() { return m_websiteDataStore.get(); }
        void addWindow(WebViewWindow* window) { m_windows.push_back(window); }
        void removeWindow(WebViewWindow* window) { m_windows.remove(window); }

        void deleteCookie();
        void deleteWebsiteData();
        void setDoNotTrackEnabled(bool);
        bool getDoNotTrackEnabled();
        void setResourceLoadStatisticsEnabled(bool);
        bool getResourceLoadStatisticsEnabled();
        void setCompositingBordersVisible(bool);
        bool getCompositingBordersVisible();
        void setTiledScrollingIndicatorVisible(bool);
        bool getTiledScrollingIndicatorVisible();
        void setLargeImageAsyncDecodingEnabled(bool);
        bool getLargeImageAsyncDecodingEnabled();
        void setVideoPlaybackRequiresUserGesture(bool);
        bool getVideoPlaybackRequiresUserGesture();
        void removeNetworkCache();

    private:
        void setNetworkProxy(const std::string* username = nullptr, const std::string* password = nullptr);

#if PLATFORM(PLAYSTATION) // for downstream-only
        void setAdditionalRootCA();
        void setClientCertificate();
#endif

        static std::weak_ptr<WebContext> s_instance;
        WKRetainPtr<WKContextRef> m_context;
        WKRetainPtr<WKPageGroupRef> m_pageGroup;
        WKRetainPtr<WKPreferencesRef> m_preferencesMaster;
        WKRetainPtr<WKWebsiteDataStoreRef> m_websiteDataStore;
        std::list<WebViewWindow*> m_windows;
    };

    struct Credential {
        std::string username;
        std::string password;
    };

    void paintSelf(toolkitten::IntPoint position) override;
    void updateSelf() override;
    void updateSurface(WKRect);
    bool isFocused() { return (focusedWidget() == this); }
    bool focusIfActive();
    void notifyViewInformation(NotifyInformations);
    void addPunchHoleWidget();
    void removePunchHoleWidget();
    void updatePunchHoles(WKPunchHole* holes, int numberOfHoles);
    void startWheelEventCapture();

    // WKViewAccessibilityClient callback.
    static void accessibilityNotification(WKViewRef, WKAXObjectRef, WKAXNotification, const void* clientInfo);
    static void accessibilityTextChanged(WKViewRef, WKAXObjectRef, WKAXTextChange, uint32_t offset, WKStringRef text, const void* clientInfo);
    static void accessibilityLoadingEvent(WKViewRef, WKAXObjectRef, WKAXLoadingEvent, const void* clientInfo);
    static void accessibilityRootObject(WKViewRef, WKAXObjectRef, const void* clientInfo);
    static void accessibilityFocusedObject(WKViewRef, WKAXObjectRef, const void* clientInfo);
    static void accessibilityHitTest(WKViewRef, WKAXObjectRef, const void* clientInfo);

    static void borderAccessibilityObject(WKAXObjectRef, const void* clientInfo);
    static void dumpAccessibilityObject(WKAXObjectRef);

    // WKViewPopupMenuClient callback.
    static void showPopupMenu(WKViewRef, WKArrayRef popupMenuItem, WKRect, int32_t selectedIndex, const void* clientInfo);
    static void hidePopupMenu(WKViewRef, const void* clientInfo);

    // shared settings (for all view, call from WebContext)
    void updateDoNotTrackEnabled();
    void updateCompositingBordersVisible();
    void updateTiledScrollingIndicatorVisible();
    void updateLargeImageAsyncDecodingEnabled();
    void updateVideoPlaybackRequiresUserGesture();

    // authentication
    std::optional<Credential> askAuthenticationCredential(WKProtectionSpaceRef);
    bool canTrustServerCertificate(WKAuthenticationChallengeRef, WKProtectionSpaceRef);
    std::string createPEMString(WKProtectionSpaceRef);

#if HAVE(OPENGL_SERVER_SUPPORT)
    // OpenGL server
    int openGLServerConnection();
    void launchOpenGLServerIfNeeded();
    void stopOpenGLServerIfNeeded();
#endif

    WKViewRef m_view;
    std::shared_ptr<WebContext> m_context;
    WKPreferencesRef m_preferences;

    std::unique_ptr<unsigned char[]> m_surface;

    std::function<void(const char*)> m_onSetTitleCallback;
    std::function<void(const char*)> m_onSetURLCallback;
    std::function<void(double)> m_onSetProgressCallback;
    OnCreateNewViewCallback m_onCreateNewViewCallback;
    std::function<void(WebViewWindow*)> m_onRequestCloseViewCallback;

    std::unique_ptr<toolkitten::ImeDialog> m_imeDialog;
    bool m_isContentEditable = false;
    bool m_isInLoginForm = false;
    WKInputFieldType m_inputFieldType = (WKInputFieldType)toolkitten::InputFieldType::Text;
    WKInputLanguage m_inputFieldLanguage = (WKInputLanguage)toolkitten::InputFieldLanguage::Japanese;
    std::string m_imeBuffer;

    std::string m_currentURL = "";
    std::string m_currentTitle = "";
    double m_currentProgress = 0;

    bool m_isActive { false };
    bool m_cookieEnabled { false };

    bool m_acceleratedCompositingEnabled { false };

    bool m_accessibilityEnabled { false };

    // m_punchHoles is sorted by when the texture should be rendered.
    // n-th element should be rendered before n+1-th element.
    std::list<PunchHoleWidget*> m_punchHoles;

    std::chrono::time_point<std::chrono::system_clock> m_lastWheelEventCaptureTime;
    bool m_drawsBackground { true };

    Menu* m_popupMenu { nullptr };

    bool m_reloadWhenWebProcessDidTerminate { false };

    FocusWidget* m_axFocusWidget { nullptr };

#if HAVE(OPENGL_SERVER_SUPPORT)
    std::unique_ptr<OpenGLServerProcessProxy> m_openGLServerProcessProxy;
#endif

    std::unordered_map<std::string, std::string> m_acceptedServerTrustCerts;

#if PLATFORM(PLAYSTATION) // for downstream-only
    WKNetworkBandwidthMode m_networkBandwidthMode { kWKNetworkBandwidthModeNormal };
#endif
};
