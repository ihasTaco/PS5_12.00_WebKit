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
#include "WebViewWindow.h"

#include "MainWindow.h"
#include "StringUtil.h"
#include <KeyboardEvents.h>
#include <WebKit/WKAXObject.h>
#include <WebKit/WKArray.h>
#include <WebKit/WKAuthenticationChallenge.h>
#include <WebKit/WKAuthenticationChallengePlayStation.h>
#include <WebKit/WKAuthenticationDecisionListener.h>
#include <WebKit/WKCertificateInfoCurl.h>
#include <WebKit/WKContextConfigurationPlayStation.h>
#include <WebKit/WKContextPlayStation.h>
#include <WebKit/WKContextPrivate.h>
#include <WebKit/WKCredential.h>
#include <WebKit/WKCredentialTypes.h>
#include <WebKit/WKData.h>
#include <WebKit/WKFrame.h>
#include <WebKit/WKFrameInfoRef.h>
#include <WebKit/WKFramePolicyListener.h>
#include <WebKit/WKHTTPCookieStoreRef.h>
#include <WebKit/WKHTTPCookieStoreRefPlayStation.h>
#include <WebKit/WKKeyValueStorageManager.h>
#include <WebKit/WKMediaKeySystemPermissionCallback.h>
#include <WebKit/WKNavigationActionRefPlayStation.h>
#include <WebKit/WKNavigationResponseRef.h>
#include <WebKit/WKNavigationResponseRefPlayStation.h>
#include <WebKit/WKNumber.h>
#include <WebKit/WKPageConfigurationRef.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPagePrivatePlayStation.h>
#include <WebKit/WKPreferencesPlayStation.h>
#include <WebKit/WKPreferencesRef.h>
#include <WebKit/WKPreferencesRefPrivate.h>
#include <WebKit/WKProtectionSpace.h>
#include <WebKit/WKProtectionSpaceCurl.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKURLRequest.h>
#include <WebKit/WKURLResponse.h>
#include <WebKit/WKView.h>
#include <WebKit/WKWebsiteDataStoreConfigurationRef.h>
#include <WebKit/WKWebsiteDataStoreRefCurl.h>
#include <WebKit/WKWebsiteDataStoreRefPlayStation.h>
#include <cairo/cairo.h>
#include <canvas/canvas.h>
#include <map>
#include <toolkitten/Application.h>
#include <toolkitten/Cursor.h>
#include <toolkitten/MessageDialog.h>

#define MAKE_PROCESS_PATH(x) "/app0/" #x ".self"

#define USE_HTTPCACHE 0

using namespace std;
using namespace toolkitten;

const int PunchHoleArrayLength = 8;

weak_ptr<WebViewWindow::WebContext> WebViewWindow::WebContext::s_instance;

const WebViewWindow::WebViewWindowFactory WebViewWindow::PlainWebViewWindowFactory = WebViewWindow::create;

static WebViewWindow* toWebView(const void* clientInfo)
{
    return const_cast<WebViewWindow*>(static_cast<const WebViewWindow*>(clientInfo));
}

static IntRect toTKRect(WKRect rect)
{
    return {
        IntPoint {
            static_cast<int>(rect.origin.x),
            static_cast<int>(rect.origin.y) },
        IntSize {
            static_cast<unsigned>(rect.size.width),
            static_cast<unsigned>(rect.size.height) }
    };
}

static IntPoint toTKIntPoint(WKPoint point)
{
    return {
        IntPoint {
            static_cast<int>(point.x),
            static_cast<int>(point.y) }
    };
}

WKRetainPtr<WKDataRef> createWKData(const unsigned char* data, size_t size)
{
    return adoptWK(WKDataCreate(data, size));
}

WebViewWindow::WebContext::WebContext()
{
    WKRetainPtr<WKContextConfigurationRef> contextConfiguration = adoptWK(WKContextConfigurationCreate());
    WKRetainPtr<WKStringRef> webProcessPath = adoptWK(WKStringCreateWithUTF8CString(MAKE_PROCESS_PATH(WebProcess)));
    WKRetainPtr<WKStringRef> networkProcessPath = adoptWK(WKStringCreateWithUTF8CString(MAKE_PROCESS_PATH(NetworkProcess)));
    WKRetainPtr<WKStringRef> cookieStoragePath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/cookie/cookie.jar.db"));
    WKRetainPtr<WKStringRef> indexedDBDatabasePath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/indexeddb"));
    WKRetainPtr<WKStringRef> localStoragePath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/local"));
    WKRetainPtr<WKStringRef> mediaKeyPath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/meidakey"));
    WKRetainPtr<WKStringRef> resourceLoadStatisticsPath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/ResourceLoadStatistics"));
    WKRetainPtr<WKStringRef> diskCachePath = adoptWK(WKStringCreateWithUTF8CString("/data/nekotests/NetworkCache"));
    WKContextConfigurationSetUserId(contextConfiguration.get(), -1);
    WKContextConfigurationSetWebProcessPath(contextConfiguration.get(), webProcessPath.get());
    WKContextConfigurationSetNetworkProcessPath(contextConfiguration.get(), networkProcessPath.get());

    auto dataStoreConfiguration = adoptWK(WKWebsiteDataStoreConfigurationCreate());

    WKWebsiteDataStoreConfigurationSetCookieStorageFile(dataStoreConfiguration.get(), cookieStoragePath.get());
    WKWebsiteDataStoreConfigurationSetIndexedDBDatabaseDirectory(dataStoreConfiguration.get(), indexedDBDatabasePath.get());
    WKWebsiteDataStoreConfigurationSetLocalStorageDirectory(dataStoreConfiguration.get(), localStoragePath.get());
    WKWebsiteDataStoreConfigurationSetMediaKeysStorageDirectory(dataStoreConfiguration.get(), mediaKeyPath.get());
    WKWebsiteDataStoreConfigurationSetResourceLoadStatisticsDirectory(dataStoreConfiguration.get(), resourceLoadStatisticsPath.get());
    WKWebsiteDataStoreConfigurationSetNetworkCacheDirectory(dataStoreConfiguration.get(), diskCachePath.get());

#if USE_HTTPCACHE
    WKCacheModel cacheModel = kWKCacheModelDocumentBrowser;
    WKWebsiteDataStoreConfigurationSetNetworkCacheSpeculativeValidationEnabled(dataStoreConfiguration.get(), true);
    WKWebsiteDataStoreConfigurationSetStaleWhileRevalidateEnabled(dataStoreConfiguration.get(), true);
#else
    WKCacheModel cacheModel = kWKCacheModelDocumentViewer;
#endif
    m_websiteDataStore = WKWebsiteDataStoreCreateWithConfiguration(dataStoreConfiguration.get());

    // Enable resourceLoadStatistics by default.
    WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(m_websiteDataStore.get(), true);

    m_context = adoptWK(WKContextCreateWithConfiguration(contextConfiguration.get()));

#if PLATFORM(PLAYSTATION) // for downstream-only
    // FIXME: Called after WKContext (WebProcessPool) generation, for network prceoss to launch. (SUPER-4731)
    WKWebsiteDataStoreSetNetworkBandwidth(m_websiteDataStore.get(), kWKNetworkBandwidthModeNormal);
#endif

    WKContextSetUsesSingleWebProcess(m_context.get(), true);
    WKContextSetCacheModel(m_context.get(), cacheModel);
    WKContextSetCanHandleHTTPSServerTrustEvaluation(m_context.get(), true);
    setNetworkProxy();
#if PLATFORM(PLAYSTATION) // for downstream-only
    setAdditionalRootCA();
    setClientCertificate();
#endif
    WKRetainPtr<WKStringRef> groupName = adoptWK(WKStringCreateWithUTF8CString("Default"));
    m_pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(groupName.get()));

    m_preferencesMaster = adoptWK(WKPreferencesCreate());

    WKPreferencesSetFullScreenEnabled(m_preferencesMaster.get(), true);
    WKPreferencesSetDNSPrefetchingEnabled(m_preferencesMaster.get(), true);
    WKPreferencesSetNeedsSiteSpecificQuirks(m_preferencesMaster.get(), false);

    // MSE/EME
    WKPreferencesSetMediaSourceEnabled(m_preferencesMaster.get(), true);
    WKPreferencesSetEncryptedMediaAPIEnabled(m_preferencesMaster.get(), true);

#if ENABLE(ACCESSIBILITY)
    WKContextSetAccessibilityEnabled(m_context.get(), false);
#endif
}

WebViewWindow::WebContext::~WebContext()
{
}

void WebViewWindow::WebContext::setNetworkProxy(const string* newUserName, const string* newPassword)
{
    static string protocol = "http";
    static string host = "";
    static string port = "80";
    static string username = "";
    static string password = "";

    if (host.empty())
        return;

    if (newUserName)
        username = *newUserName;

    if (newPassword)
        password = *newPassword;

    auto userpass = (!username.empty() || !password.empty()) ? std::string(username + ":" + password + "@") : std::string("");
    auto proxyURL = protocol + "://" + userpass + host + ":" + port + "/";

    auto wkProxyURL = adoptWK(WKURLCreateWithUTF8CString(proxyURL.c_str()));
    auto wkExcludeHosts = adoptWK(WKStringCreateWithUTF8CString(""));

    WKWebsiteDataStoreEnableCustomNetworkProxySettings(websiteDataStore(), wkProxyURL.get(), wkExcludeHosts.get());
}

#if PLATFORM(PLAYSTATION) // for downstream-only
void WebViewWindow::WebContext::setAdditionalRootCA()
{
    static const string additionalRootCA = "";

    if (additionalRootCA.empty())
        return;

    auto wkAdditionalRootCA = adoptWK(WKStringCreateWithUTF8CString(additionalRootCA.c_str()));

    WKWebsiteDataStoreSetAdditionalRootCA(websiteDataStore(), wkAdditionalRootCA.get());
}

void WebViewWindow::WebContext::setClientCertificate()
{
    static const std::string certificate = "";
    static const std::string privateKey = "";
    static const std::string privateKeyPassword = "";

    if (certificate.empty())
        return;

    auto wkCertificate = createWKData(reinterpret_cast<const unsigned char*>(certificate.data()), certificate.size());
    auto wkPrivateKey = createWKData(reinterpret_cast<const unsigned char*>(privateKey.data()), privateKey.size());
    auto wkPrivateKeyPassword = adoptWK(WKStringCreateWithUTF8CString(privateKeyPassword.c_str()));

    WKWebsiteDataStoreSetClientCertificateV2(websiteDataStore(), wkCertificate.get(), wkPrivateKey.get(), wkPrivateKeyPassword.get());
}
#endif

void WebViewWindow::WebContext::deleteCookie()
{
    auto cookieStore = WKWebsiteDataStoreGetHTTPCookieStore(websiteDataStore());
    if (cookieStore)
        WKHTTPCookieStoreDeleteAllCookies(cookieStore, nullptr, nullptr);
}

static void removeLocalStorageCallback(void*)
{

}

static void resourceStatisticsVoidResultCallback(void*)
{
}

void WebViewWindow::WebContext::deleteWebsiteData()
{
    // local storage
    WKWebsiteDataStoreRemoveLocalStorage(websiteDataStore(), this, removeLocalStorageCallback);

    // website data store
    bool needTemporaryEnabled = !WKWebsiteDataStoreGetResourceLoadStatisticsEnabled(websiteDataStore());

    // Enable "ResourceLoadStatistics" (ITP) temporary to delete web site data forcefully.
    if (needTemporaryEnabled)
        WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(websiteDataStore(), true);

    WKWebsiteDataStoreStatisticsResetToConsistentState(websiteDataStore(), this, resourceStatisticsVoidResultCallback);

    // reset state.
    if (needTemporaryEnabled)
        WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(websiteDataStore(), false);
}

void WebViewWindow::WebContext::setResourceLoadStatisticsEnabled(bool enable)
{
    WKWebsiteDataStoreSetResourceLoadStatisticsEnabled(websiteDataStore(), enable);
}

bool WebViewWindow::WebContext::getResourceLoadStatisticsEnabled()
{
    return WKWebsiteDataStoreGetResourceLoadStatisticsEnabled(websiteDataStore());
}

void WebViewWindow::WebContext::setCompositingBordersVisible(bool visible)
{
    WKPreferencesSetCompositingBordersVisible(m_preferencesMaster.get(), visible);
    WKPreferencesSetCompositingRepaintCountersVisible(m_preferencesMaster.get(), visible);

    for (auto& view : m_windows)
        view->updateCompositingBordersVisible();
}

bool WebViewWindow::WebContext::getCompositingBordersVisible()
{
    return WKPreferencesGetCompositingBordersVisible(m_preferencesMaster.get());
}

void WebViewWindow::WebContext::setTiledScrollingIndicatorVisible(bool visible)
{
    WKPreferencesSetTiledScrollingIndicatorVisible(m_preferencesMaster.get(), visible);

    for (auto& view : m_windows)
        view->updateTiledScrollingIndicatorVisible();
}

bool WebViewWindow::WebContext::getTiledScrollingIndicatorVisible()
{
    return WKPreferencesGetTiledScrollingIndicatorVisible(m_preferencesMaster.get());
}

void WebViewWindow::WebContext::setLargeImageAsyncDecodingEnabled(bool enabled)
{
    WKPreferencesSetLargeImageAsyncDecodingEnabled(m_preferencesMaster.get(), enabled);

    for (auto& view : m_windows)
        view->updateLargeImageAsyncDecodingEnabled();
}

bool WebViewWindow::WebContext::getLargeImageAsyncDecodingEnabled()
{
    return WKPreferencesGetLargeImageAsyncDecodingEnabled(m_preferencesMaster.get());
}

void WebViewWindow::WebContext::setVideoPlaybackRequiresUserGesture(bool enabled)
{
    WKPreferencesSetVideoPlaybackRequiresUserGesture(m_preferencesMaster.get(), enabled);

    for (auto& view : m_windows)
        view->updateVideoPlaybackRequiresUserGesture();
}

bool WebViewWindow::WebContext::getVideoPlaybackRequiresUserGesture()
{
    return WKPreferencesGetVideoPlaybackRequiresUserGesture(m_preferencesMaster.get());
}

static void removeNetworkCacheCallback(void*)
{
    printf("removeNetworkCacheCallback\n");
}

void WebViewWindow::WebContext::removeNetworkCache()
{
    WKWebsiteDataStoreRemoveNetworkCache(websiteDataStore(), this, removeNetworkCacheCallback);
}

std::unique_ptr<WebViewWindow> WebViewWindow::create()
{
    auto context = WebContext::singleton();

    WKRetainPtr<WKPageConfigurationRef> configuration = adoptWK(WKPageConfigurationCreate());
    WKPageConfigurationSetContext(configuration.get(), context->context());
    WKPageConfigurationSetPageGroup(configuration.get(), context->pageGroup());
    WKPageConfigurationSetWebsiteDataStore(configuration.get(), context->websiteDataStore());

    return std::make_unique<WebViewWindow>(configuration.get());
}

void assertReportCallback(const char* message, const unsigned line)
{
    printf("### Assert report callback pid:%d ###\n   [%s:%d]\n", getpid(), message, line);
}

WebViewWindow::WebViewWindow(WKPageConfigurationRef configuration)
{
    // FIXME: Should child share the preference with parent?
    m_context = WebContext::singleton();
    m_preferences = WKPreferencesCreateCopy(m_context->preferences());
    WKPageConfigurationSetPreferences(configuration, m_preferences);

    // We should complete all the preference setup before WKViewCreate.
    setAcceleratedCompositing(false); // Disable AC by default.

    WKContextSetAssertReportCallback(m_context->context(), assertReportCallback);

    m_view = WKViewCreate(configuration);
    WKViewClientV1 client {
        { 1, this },
        viewNeedsDisplay,
        enterFullScreen,
        exitFullScreen,
        nullptr, // closeFullScreen
        nullptr, // beganEnterFullScreen
        nullptr, // beganExitFullScreen
        doneWithMouseUpEvent, // doneWithMouseUpEvent
        nullptr, // doneWithKeyboardEvent
        didChangeEditorState, // didChangeEditorState
        nullptr, // didChangeSelectionState
        nullptr, // didChangeCompositionState
        setCursorPosition, // setCursorPosition
        setCursor, // setCursor
        enterAcceleratedCompositingMode, // enterAcceleratedCompositingMode
        exitAcceleratedCompositingMode, // exitAcceleratedCompositingMode
        didChangeContentsVisibility, // didChangeContentsVisibility
        launchOpenGLServerProcess
    };
    WKViewSetViewClient(m_view, &client.base);

    WKPageNavigationClientV1 navigationClient = {
        { 1, this },

        decidePolicyForNavigationAction,
        decidePolicyForNavigationResponse,
        nullptr, // decidePolicyForPluginLoad
        nullptr, // didStartProvisionalNavigation,
        nullptr, // didReceiveServerRedirectForProvisionalNavigation
        nullptr, // didFailProvisionalNavigation,
        nullptr, // didCommitNavigation,
        nullptr, // didFinishNavigation,
        nullptr, // didFailNavigation,
        nullptr, // didFailProvisionalLoadInSubframe
        nullptr, // didFinishDocumentLoad
        nullptr, // didSameDocumentNavigation,
        nullptr, // renderingProgressDidChange
        nullptr, // canAuthenticateAgainstProtectionSpace
        didReceiveAuthenticationChallenge, // didReceiveAuthenticationChallenge,
        nullptr, // webProcessDidCrash
        nullptr, // copyWebCryptoMasterKey
        nullptr, // didBeginNavigationGesture
        nullptr, // willEndNavigationGesture
        nullptr, // didEndNavigationGesture
        nullptr, // didRemoveNavigationGestureSnapshot
        webProcessDidTerminate, // webProcessDidTerminate
    };
    WKPageUIClientV16 uiClient {
        { 16, this },
        nullptr, // createNewPage_deprecatedForUseWithV0
        nullptr, // showPage
        close, // close
        nullptr, // takeFocus
        nullptr, // focus
        nullptr, // unfocus
        nullptr, // runJavaScriptAlert_deprecatedForUseWithV0
        nullptr, // runJavaScriptConfirm_deprecatedForUseWithV0
        nullptr, // runJavaScriptPrompt_deprecatedForUseWithV0
        nullptr, // setStatusText
        nullptr, // mouseDidMoveOverElement_deprecatedForUseWithV0
        nullptr, // missingPluginButtonClicked_deprecatedForUseWithV0
        nullptr, // didNotHandleKeyEvent
        didNotHandleWheelEvent, // didNotHandleWheelEvent
        nullptr, // toolbarsAreVisible
        nullptr, // setToolbarsAreVisible
        nullptr, // menuBarIsVisible
        nullptr, // setMenuBarIsVisible
        nullptr, // statusBarIsVisible
        nullptr, // setStatusBarIsVisible
        nullptr, // isResizable
        nullptr, // setIsResizable
        getWindowFrame, // getWindowFrame
        nullptr, // setWindowFrame
        nullptr, // runBeforeUnloadConfirmPanel
        nullptr, // didDraw
        nullptr, // pageDidScroll
        nullptr, // exceededDatabaseQuota
        nullptr, // runOpenPanel
        nullptr, // decidePolicyForGeolocationPermissionRequest
        nullptr, // headerHeight
        nullptr, // footerHeight
        nullptr, // drawHeader
        nullptr, // drawFooter
        nullptr, // printFrame
        nullptr, // runModal
        nullptr, // Used to be didCompleteRubberBandForMainFrame
        nullptr, // saveDataToFileInDownloadsFolder
        nullptr, // shouldInterruptJavaScript_unavailable
        nullptr, // createNewPage_deprecatedForUseWithV1
        nullptr, // mouseDidMoveOverElement
        nullptr, // decidePolicyForNotificationPermissionRequest
        nullptr, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        nullptr, // showColorPicker
        nullptr, // hideColorPicker
        nullptr, // unavailablePluginButtonClicked
        nullptr, // pinnedStateDidChange
        nullptr, // Used to be didBeginTrackingPotentialLongMousePress
        nullptr, // Used to be didRecognizeLongMousePress
        nullptr, // Used to be didCancelTrackingPotentialLongMousePress
        nullptr, // isPlayingAudioDidChange
        nullptr, // decidePolicyForUserMediaPermissionRequest
        nullptr, // didClickAutoFillButton
        nullptr, // runJavaScriptAlert_deprecatedForUseWithV5
        nullptr, // runJavaScriptConfirm_deprecatedForUseWithV5
        nullptr, // runJavaScriptPrompt_deprecatedForUseWithV5
        nullptr, // mediaSessionMetadataDidChange
        createNewPage, // createNewPage
        runJavaScriptAlert, // runJavaScriptAlert
        nullptr, // runJavaScriptConfirm
        nullptr, // runJavaScriptPrompt
        nullptr, // checkUserMediaPermissionForOrigin
        nullptr, // runBeforeUnloadConfirmPanel
        nullptr, // fullscreenMayReturnToInline
        nullptr, // requestPointerLock
        nullptr, // didLosePointerLock
        nullptr, // handleAutoplayEvent
        nullptr, // hasVideoInPictureInPictureDidChange
        nullptr, // didExceedBackgroundResourceLimitWhileInForeground
        nullptr, // didResignInputElementStrongPasswordAppearance
        nullptr, // requestStorageAccessConfirm
        nullptr, // shouldAllowDeviceOrientationAndMotionAccess
        nullptr, // runWebAuthenticationPanel
        nullptr, // decidePolicyForSpeechRecognitionPermissionRequest
        decidePolicyForMediaKeySystemPermissionRequest // decidePolicyForMediaKeySystemPermissionRequest
    };

    WKPageStateClientV0 stateClient {
        {0, this},
        nullptr, // willChangeIsLoading;
        nullptr, // didChangeIsLoading;
        nullptr, // willChangeTitle;
        didChangeTitle, // didChangeTitle;
        nullptr, // willChangeActiveURL;
        didChangeActiveURL, // didChangeActiveURL;
        nullptr, // willChangeHasOnlySecureContent;
        nullptr, // didChangeHasOnlySecureContent;
        nullptr, // willChangeEstimatedProgress;
        didChangeEstimatedProgress, // didChangeEstimatedProgress;
        nullptr, // willChangeCanGoBack;
        nullptr, // didChangeCanGoBack;
        nullptr, // willChangeCanGoForward;
        nullptr, // didChangeCanGoForward;
        nullptr, // willChangeNetworkRequestsInProgress;
        nullptr, // didChangeNetworkRequestsInProgress;
        nullptr, // willChangeCertificateInfo;
        nullptr, // didChangeCertificateInfo;
        nullptr, // willChangeWebProcessIsResponsive;
        didChangeWebProcessIsResponsive, // didChangeWebProcessIsResponsive;
        didSwapWebProcesses, // didSwapWebProcesses;
    };

#if ENABLE(ACCESSIBILITY)
    WKViewAccessibilityClientV0 viewAccessibilityClient {
        { 0, this },
        accessibilityNotification,
        accessibilityTextChanged,
        accessibilityLoadingEvent,
        accessibilityRootObject,
        accessibilityFocusedObject,
        accessibilityHitTest
    };
#endif

    WKViewPopupMenuClientV0 popupMenuClient = {
        { 0, this },
        showPopupMenu,
        hidePopupMenu
    };

    WKPageRef page = WKViewGetPage(m_view);
    m_context->addWindow(this);
    WKPageSetPageNavigationClient(page, &navigationClient.base);
    WKPageSetPageUIClient(page, &uiClient.base);
    WKPageSetPageStateClient(page, &stateClient.base);
#if ENABLE(ACCESSIBILITY)
    WKViewSetViewAccessibilityClient(m_view, &viewAccessibilityClient.base);
#endif
    WKViewSetViewPopupMenuClient(m_view, &popupMenuClient.base);
    WKPageSetDrawsBackground(WKViewGetPage(m_view), m_drawsBackground);

    updateCookieAcceptPolicy(kWKHTTPCookieAcceptPolicyAlways);

    auto focusWidget = std::make_unique<FocusWidget>();
    m_axFocusWidget = focusWidget.get();
    m_axFocusWidget->setPosition({ 0 , 0 });
    m_axFocusWidget->setSize({ 0, 0 });
    appendChild(std::move(focusWidget));
}

WebViewWindow::~WebViewWindow()
{
    m_context->removeWindow(this);
    WKPageClose(WKViewGetPage(m_view));
    WKRelease(m_preferences);
    WKRelease(m_view);
}

void WebViewWindow::loadURL(const char* url)
{
    WKRetainPtr<WKURLRef> urlStr = adoptWK(WKURLCreateWithUTF8CString(url));

    if (std::string(url) == "about:") {
        std::string forAbout = R"(
                                <html>
                                    <head>
                                        <meta charset='utf-8'>
                                        <script type='text/javascript'>
                                            window.onload = function() {
                                                document.getElementById("userAgent").textContent = navigator.userAgent;
                                            }
                                        </script>
                                    </head>

                                    <body>
                                        <table>
                                            <tr><td>UserAgent : </td><td id='userAgent'></tr>
                                        </table>
                                    </body>
                                </html> )";

        WKPageLoadHTMLString(WKViewGetPage(m_view), adoptWK(WKStringCreateWithUTF8CString(forAbout.c_str())).get(), urlStr.get());
    } else
        WKPageLoadURL(WKViewGetPage(m_view), urlStr.get());
}

void WebViewWindow::goBack()
{
    WKPageGoBack(WKViewGetPage(m_view));
}

void WebViewWindow::goForward()
{
    WKPageGoForward(WKViewGetPage(m_view));
}

void WebViewWindow::reload()
{
    WKPageReloadFromOrigin(WKViewGetPage(m_view));
}

void WebViewWindow::setActive(bool active)
{
    if (m_isActive == active)
        return;
    WKViewSetFocus(m_view, active);
    WKViewSetActive(m_view, active);
    WKViewSetVisible(m_view, active);

    if (active)
        WKPageResumeActiveDOMObjectsAndAnimations(WKViewGetPage(m_view));
    else
        WKPageSuspendActiveDOMObjectsAndAnimations(WKViewGetPage(m_view));

    m_isActive = active;
    focusIfActive();
    notifyViewInformation(kNotifyALL);
}

void WebViewWindow::setUserAgent(const char* userAgent)
{
    WKRetainPtr<WKStringRef> uaStr = adoptWK(WKStringCreateWithUTF8CString(userAgent));
    WKPageSetCustomUserAgent(WKViewGetPage(m_view), uaStr.get());
}

std::string WebViewWindow::getUserAgent()
{
    WKRetainPtr<WKStringRef> uaStr = adoptWK(WKPageCopyUserAgent(WKViewGetPage(m_view)));
    return std::string(toUTF8String(uaStr.get()).c_str());
}

void WebViewWindow::setOnSetTitleCallBack(std::function<void(const char*)>&& onSetTitleCallback)
{
    m_onSetTitleCallback = std::move(onSetTitleCallback);
}

void WebViewWindow::setOnSetURLCallBack(std::function<void(const char*)>&& onSetURLCallback)
{
    m_onSetURLCallback = std::move(onSetURLCallback);
}

void WebViewWindow::setOnSetProgressCallBack(std::function<void(double)>&& onSetProgressCallback)
{
    m_onSetProgressCallback = std::move(onSetProgressCallback);
}

void WebViewWindow::setOnCreateNewViewCallback(OnCreateNewViewCallback&& onCreateNewViewCallback)
{
    m_onCreateNewViewCallback = std::move(onCreateNewViewCallback);
}

void WebViewWindow::setOnRequestCloseViewCallback(std::function<void(WebViewWindow*)>&& onRequestCloseViewCallback)
{
    m_onRequestCloseViewCallback = std::move(onRequestCloseViewCallback);
}

void WebViewWindow::setSize(toolkitten::IntSize size)
{
    Widget::setSize(size);

    size_t surfaceSize = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, this->m_size.w) * this->m_size.h;
    m_surface = std::make_unique<unsigned char[]>(surfaceSize);
    // TODO: If we need to set `drawBackground` to false, we must fill m_surface with 0x00 instead of 0xff.
    memset(m_surface.get(), 0xff, surfaceSize);

    WKViewSetSize(m_view, WKSizeMake(this->m_size.w, this->m_size.h));
}

bool WebViewWindow::onKeyUp(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_RETURN:
        return false;
    case VK_TRIANGLE:
    case VK_SQUARE:
    case VK_OPTIONS:
    case VK_L1:
    case VK_R1:
    case VK_L2:
    case VK_R2:
    case VK_L3:
    case VK_R3:
        // Ignore these key code.
        return false;
    }

    WKPageHandleKeyboardEvent(WKViewGetPage(m_view), WKKeyboardEventMake(kWKEventKeyUp, kWKInputTypeNormal, "", 0, keyIdentifierForKeyCode(virtualKeyCode), virtualKeyCode, "", "", -1, 0, 0));
    return true;
}

bool WebViewWindow::onKeyDown(int32_t virtualKeyCode)
{
    switch (virtualKeyCode) {
    case VK_RETURN:
        return false;
    case VK_TRIANGLE:
    case VK_SQUARE:
    case VK_OPTIONS:
    case VK_L1:
    case VK_R1:
    case VK_L2:
    case VK_R2:
    case VK_L3:
    case VK_R3:
        // Ignore these key code.
        return false;
    }
    WKPageHandleKeyboardEvent(WKViewGetPage(m_view), WKKeyboardEventMake(kWKEventKeyDown, kWKInputTypeNormal, "", 0, keyIdentifierForKeyCode(virtualKeyCode), virtualKeyCode, "", "", -1, 0, 0));
    return true;
}

bool WebViewWindow::onMouseMove(toolkitten::IntPoint point)
{
    if (!focusIfActive())
        return false;
    point = globalToClientPosition(point);
    WKPageHandleMouseEvent(WKViewGetPage(m_view), WKMouseEventMake(kWKEventMouseMove, kWKEventMouseButtonNoButton, WKPointMake(point.x, point.y), 0));
#if ENABLE(ACCESSIBILITY)
    WKViewAccessibilityHitTest(m_view, WKPointMake(point.x, point.y));
#endif
    return true;
}

bool WebViewWindow::onMouseDown(toolkitten::IntPoint point)
{
    if (!focusIfActive())
        return false;
    point = globalToClientPosition(point);

    WKPageHandleMouseEvent(WKViewGetPage(m_view), WKMouseEventMake(kWKEventMouseDown, kWKEventMouseButtonLeftButton, WKPointMake(point.x, point.y), kWKMouseEventModifiersLeftButton));
    if (MainWindow* mainWindow = static_cast<MainWindow*>(userData()))
        mainWindow->closeDropdownMenu();
    return true;
}

bool WebViewWindow::onMouseUp(toolkitten::IntPoint point)
{
    if (!focusIfActive())
        return false;

    point = globalToClientPosition(point);
    WKPageHandleMouseEvent(WKViewGetPage(m_view), WKMouseEventMake(kWKEventMouseUp, kWKEventMouseButtonLeftButton, WKPointMake(point.x, point.y), 0));
#if ENABLE(ACCESSIBILITY)
    WKViewAccessibilityFocusedObject(m_view);
#endif
    return true;
}

bool WebViewWindow::onWheelMove(toolkitten::IntPoint point, toolkitten::IntPoint wheelTicks)
{
    if (!isFocused())
        return false;

    point = globalToClientPosition(point);

    int wheelTicksX = wheelTicks.x;
    int wheelTicksY = wheelTicks.y;
    if (wheelTicksX || wheelTicksY) {
        // same ratio as 601Manx
        int deltaX = wheelTicksX * 10;
        int deltaY = wheelTicksY * 10;

        WKPageHandleWheelEvent(WKViewGetPage(m_view), WKWheelEventMake(kWKEventWheel, WKPointMake(point.x, point.y), WKSizeMake(deltaX, deltaY), WKSizeMake(wheelTicksX, wheelTicksY), 0));
        return true;
    }
    return false;
}

void WebViewWindow::paintSelf(IntPoint position)
{
    if (!m_isActive)
        return;
    if (!dirtyRects().empty()) {
        cairo_surface_t* wkviewSurface = cairo_image_surface_create_for_data(m_surface.get(), CAIRO_FORMAT_ARGB32, this->m_size.w, this->m_size.h, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, this->m_size.w));
        std::list<toolkitten::IntRect>::const_iterator it = dirtyRects().begin();
        toolkitten::IntRect unionRect = *it;
        it++;
        while (it != dirtyRects().end()) {
            unionRect += *it;
            it++;
        }
        if (cairo_surface_status(wkviewSurface) == CAIRO_STATUS_SUCCESS) {
            cairo_t* cr = cairo_create(this->surface());
            cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(cr, wkviewSurface, 0, 0);
            cairo_rectangle(cr, unionRect.left(), unionRect.top(), unionRect.width(), unionRect.height());
            cairo_fill(cr);
            cairo_destroy(cr);
            cairo_surface_destroy(wkviewSurface);
        }
    }
    Widget::paintSelf(position);
}

void WebViewWindow::updateSelf()
{
    if (focusedWidget() == this) {
        if (m_imeDialog) {
            if (m_imeDialog->isFinished()) {
                m_imeBuffer = m_imeDialog->getImeText();
                WKImeEvent* imeEvent = WKImeEventMake(SetValueToFocusedNode, m_imeBuffer.c_str(), 0);
                WKPageHandleImeEvent(WKViewGetPage(m_view), imeEvent);
                WKImeEventRelease(imeEvent);
                m_imeDialog.reset();
            }
        }
    }
}

void WebViewWindow::updateSurface(WKRect dirtyRect)
{
    WKPagePaint(WKViewGetPage(m_view), m_surface.get(), WKSizeMake(this->m_size.w, this->m_size.h), dirtyRect);
    m_dirtyRects += toTKRect(dirtyRect);

    WKPunchHole holes[PunchHoleArrayLength];
    int numberOfCopiedPunchHoles = 0;
    int numberOfActualPunchHoles = 0;
    WKViewGetPunchHoles(m_view, holes, PunchHoleArrayLength, &numberOfCopiedPunchHoles, &numberOfActualPunchHoles);

    ASSERT(numberOfActualPunchHoles <= PunchHoleArrayLength);

    updatePunchHoles(holes, numberOfCopiedPunchHoles);
}

bool WebViewWindow::focusIfActive()
{
    if (m_isActive && !isFocused())
        setFocused();
    return isFocused();
}

void WebViewWindow::notifyViewInformation(NotifyInformations informations)
{
    if (!m_isActive)
        return;

    if ((informations & kNotifyURL) && m_onSetURLCallback)
        m_onSetURLCallback(m_currentURL.c_str());

    if ((informations & kNotifyTitle) && m_onSetTitleCallback)
        m_onSetTitleCallback(m_currentTitle.c_str());

    if ((informations & kNotifyProgress) && m_onSetProgressCallback)
        m_onSetProgressCallback(m_currentProgress);
}

void WebViewWindow::viewNeedsDisplay(WKViewRef, WKRect rect, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    webView->updateSurface(rect);
}

void WebViewWindow::enterFullScreen(WKViewRef view, const void*)
{
    WKViewWillEnterFullScreen(view);
    WKViewDidEnterFullScreen(view);
}

void WebViewWindow::exitFullScreen(WKViewRef view, const void*)
{
    WKViewWillExitFullScreen(view);
    WKViewDidExitFullScreen(view);
}

void WebViewWindow::doneWithMouseUpEvent(WKViewRef view, bool, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);

    if (!webView->m_isContentEditable)
        return;
    WKPageSetCaretVisible(WKViewGetPage(view), true);

    EditorState editorState;
    editorState.inputType = (InputFieldType)webView->m_inputFieldType;
    editorState.supportLanguage = (InputFieldLanguage)webView->m_inputFieldLanguage;
    editorState.title = "WebView Input Text";
    editorState.text = webView->m_imeBuffer;
    editorState.position = webView->globalPosition();
    editorState.isInLoginForm = webView->m_isInLoginForm;
    webView->m_imeDialog = ImeDialog::create(editorState);
    if (!webView->m_imeDialog)
        fprintf(stderr, "ERROR: ImeDialog::create() WebView Input Text\n");
}

void WebViewWindow::setCursorPosition(WKViewRef, WKPoint cursorPosition, const void* clientInfo)
{
    WebViewWindow* thiz = toWebView(clientInfo);

    IntPoint globalpos = thiz->clientToGlobalPosition(toTKIntPoint(cursorPosition));
    Cursor::singleton().moveTo(globalpos);
}

void WebViewWindow::setCursor(WKViewRef, WKCursorType cursorType, const void*)
{
    switch (cursorType) {
    case kWKCursorTypeHand:
        Cursor::singleton().setType(Cursor::kTypeHand);
        break;
    case kWKCursorTypeIBeam:
        Cursor::singleton().setType(Cursor::kTypeIBeam);
        break;
    case kWKCursorTypePointer:
    default:
        Cursor::singleton().setType(Cursor::kTypePointer);
        break;
    }
}

void WebViewWindow::didChangeEditorState(WKViewRef view, WKEditorState editorState, WKRect, WKStringRef inputText, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);

    webView->m_isContentEditable = editorState.contentEditable;
    if (!webView->m_isContentEditable)
        return;
    WKPageSetCaretVisible(WKViewGetPage(view), false);
    webView->m_imeBuffer = toUTF8String(inputText);
    webView->m_inputFieldType = editorState.type;
    webView->m_inputFieldLanguage = editorState.lang;
    webView->m_isInLoginForm = editorState.isInLoginForm;
}

void WebViewWindow::decidePolicyForNavigationAction(WKPageRef, WKNavigationActionRef navigationAction, WKFramePolicyListenerRef listener, WKTypeRef, const void*)
{
    WKRetainPtr<WKURLRequestRef> request = adoptWK(WKNavigationActionCopyURLRequest(navigationAction));
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLRequestCopyURL(request.get()));
    WKRetainPtr<WKStringRef> urlStr = adoptWK(WKURLCopyString(url.get()));

    printf("%s: %s: %s: %s: %d\n", __FUNCTION__
        , toUTF8String(urlStr.get()).c_str()
        , WKNavigationActionIsNewWindowAction(navigationAction) ? "NewWindowAction" : "NavigationAction"
        , WKNavigationActionIsMainFrame(navigationAction) ? "MainFrame" : "SubFrame"
        , WKNavigationActionGetNavigationType(navigationAction)
        );
    WKFramePolicyListenerUse(listener);
}

void WebViewWindow::decidePolicyForNavigationResponse(WKPageRef, WKNavigationResponseRef navigationResponse, WKFramePolicyListenerRef listener, WKTypeRef, const void*)
{
    WKRetainPtr<WKURLResponseRef> response = adoptWK(WKNavigationResponseCopyURLResponse(navigationResponse));
    WKRetainPtr<WKFrameInfoRef> frameInfo = adoptWK(WKNavigationResponseCopyFrameInfo(navigationResponse));
    WKRetainPtr<WKURLRef> url = adoptWK(WKURLResponseCopyURL(response.get()));
    WKRetainPtr<WKStringRef> urlStr = adoptWK(WKURLCopyString(url.get()));
    WKRetainPtr<WKStringRef> mimeType = adoptWK(WKURLResponseCopyMIMEType(response.get()));

    printf("%s: %s: %s: %s: %d\n", __FUNCTION__
        , toUTF8String(urlStr.get()).c_str()
        , toUTF8String(mimeType.get()).c_str()
        , WKFrameInfoGetIsMainFrame(frameInfo.get()) ? "MainFrame" : "SubFrame"
        , WKURLResponseHTTPStatusCode(response.get())
    );

    if (WKNavigationResponseCanShowMIMEType(navigationResponse))
        WKFramePolicyListenerUse(listener);
    else {
        std::string message("Not supported MIMEType! : ");
        message.append(toUTF8String(mimeType.get()));
        MessageDialog::showModal(message.c_str());
        WKFramePolicyListenerIgnore(listener);
    }
}

void WebViewWindow::didReceiveAuthenticationChallenge(WKPageRef, WKAuthenticationChallengeRef authenticationChallenge, const void* clientInfo)
{
    printf("didReceiveAuthenticationChallenge from %s\n", WKAuthenticationChallengeIsInMainFrame(authenticationChallenge) ? "MainFrame" : "SubFrame");
    WebViewWindow* webView = toWebView(clientInfo);
    auto protectionSpace = WKAuthenticationChallengeGetProtectionSpace(authenticationChallenge);
    auto decisionListener = WKAuthenticationChallengeGetDecisionListener(authenticationChallenge);
    auto authenticationScheme = WKProtectionSpaceGetAuthenticationScheme(protectionSpace);

    if (authenticationScheme == kWKProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested) {
        if (webView->canTrustServerCertificate(authenticationChallenge, protectionSpace)) {
            WKRetainPtr<WKStringRef> username = adoptWK(WKStringCreateWithUTF8CString("accept server trust"));
            WKRetainPtr<WKStringRef> password = adoptWK(WKStringCreateWithUTF8CString(""));
            WKRetainPtr<WKCredentialRef> wkCredential = adoptWK(WKCredentialCreate(username.get(), password.get(), kWKCredentialPersistenceForSession));
            WKAuthenticationDecisionListenerUseCredential(decisionListener, wkCredential.get());
            return;
        }
    } else if (authenticationScheme == kWKProtectionSpaceAuthenticationSchemeHTTPBasic || authenticationScheme == kWKProtectionSpaceAuthenticationSchemeHTTPDigest) {
        if (auto credential = webView->askAuthenticationCredential(protectionSpace)) {
            WKRetainPtr<WKStringRef> username = adoptWK(WKStringCreateWithUTF8CString(credential->username.c_str()));
            WKRetainPtr<WKStringRef> password = adoptWK(WKStringCreateWithUTF8CString(credential->password.c_str()));
            WKRetainPtr<WKCredentialRef> wkCredential = adoptWK(WKCredentialCreate(username.get(), password.get(), kWKCredentialPersistenceForSession));
            WKAuthenticationDecisionListenerUseCredential(decisionListener, wkCredential.get());
            return;
        }
    }

    WKAuthenticationDecisionListenerUseCredential(decisionListener, nullptr);
}

void WebViewWindow::webProcessDidTerminate(WKPageRef, WKProcessTerminationReason reason, const void* clientInfo)
{
    printf("webProcessDidTerminate by %d\n", reason);

    WebViewWindow* webView = toWebView(clientInfo);
    if (webView->getReloadWhenWebProcessDidTerminate())
        webView->reload();
}

std::optional<WebViewWindow::Credential> WebViewWindow::askAuthenticationCredential(WKProtectionSpaceRef protectionSpace)
{
    auto isProxy = WKProtectionSpaceGetIsProxy(protectionSpace);
    Credential result;

    IntSize size = Application::singleton().windowSize();
    EditorState editorState;
    editorState.inputType = InputFieldType::Text;
    editorState.supportLanguage = InputFieldLanguage::English_Us;
    editorState.text = "";
    editorState.position = { static_cast<int>(size.w / 3), static_cast<int>(size.h / 3) };
    editorState.isInLoginForm = false;

    editorState.title = (isProxy ? "Proxy" : "HTTP") + string(" Auth : Username");
    if (!ImeDialog::showModal(editorState, result.username))
        return std::nullopt;

    editorState.title = (isProxy ? "Proxy" : "HTTP") + string(" Auth : Password");
    editorState.inputType = InputFieldType::PassWord;
    if (!ImeDialog::showModal(editorState, result.password))
        return std::nullopt;

    return result;
}

bool WebViewWindow::canTrustServerCertificate(WKAuthenticationChallengeRef authenticationChallenge, WKProtectionSpaceRef protectionSpace)
{
    auto host = toUTF8String(adoptWK(WKProtectionSpaceCopyHost(protectionSpace)).get());
    auto pems = createPEMString(protectionSpace);

    auto it = m_acceptedServerTrustCerts.find(host);
    if (it != m_acceptedServerTrustCerts.end() && it->second == pems)
        return true;

    auto isForMainResource = WKAuthenticationChallengeIsForMainResource(authenticationChallenge);
    if (!isForMainResource)
        return false;

    printf("### Server trust evaluation\n");
    printf("host = %s\n", host.c_str());
    printf("pems = %s\n", pems.c_str());

    auto result = MessageDialog::showModal("Server trust evaluation failed. Accept the server trust?");
    if (result != MessageDialog::Result::kOK)
        return false;

    m_acceptedServerTrustCerts.emplace(host, pems);
    return true;
}

std::string WebViewWindow::createPEMString(WKProtectionSpaceRef protectionSpace)
{
    auto certificateInfo = WKProtectionSpaceCopyCertificateInfo(protectionSpace);
    auto chainSize = WKCertificateInfoGetCertificateChainSize(certificateInfo);

    std::string pems;

    for (auto i = 0u; i < chainSize; i++) {
        auto certificate = adoptWK(WKCertificateInfoCopyCertificateAtIndex(certificateInfo, i));
        auto size = WKDataGetSize(certificate.get());
        auto data = WKDataGetBytes(certificate.get());
        pems.append(std::string(data, data + size));
    }

    return pems;
}

WKPageRef WebViewWindow::createNewPage(WKPageRef, WKPageConfigurationRef configuration, WKNavigationActionRef, WKWindowFeaturesRef, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    WKPageRef newPage = nullptr;

    auto factory = [configuration]() {
        return std::make_unique<WebViewWindow>(configuration);
    };

    if (webView->m_onCreateNewViewCallback) {
        if (WebViewWindow* newView = webView->m_onCreateNewViewCallback(factory)) {
            newPage = WKViewGetPage(newView->m_view);
            WKRetain(newPage);
        }
    }
    return newPage;
}

void WebViewWindow::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, WKSecurityOriginRef, WKPageRunJavaScriptAlertResultListenerRef listener, const void*)
{
    MessageDialog::showModal(toUTF8String(alertText).c_str());
    WKPageRunJavaScriptAlertResultListenerCall(listener);
}

void WebViewWindow::close(WKPageRef, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    if (webView->m_onRequestCloseViewCallback)
        webView->m_onRequestCloseViewCallback(webView);
}

void WebViewWindow::decidePolicyForMediaKeySystemPermissionRequest(WKPageRef, WKSecurityOriginRef, WKStringRef, WKMediaKeySystemPermissionCallbackRef callback)
{
    WKMediaKeySystemPermissionCallbackComplete(callback, true);
}

void WebViewWindow::didNotHandleWheelEvent(WKPageRef, WKNativeEventPtr, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    webView->startWheelEventCapture();
}

WKRect WebViewWindow::getWindowFrame(WKPageRef, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    return WKRectMake(webView->position().x, webView->position().y, webView->size().w, webView->size().h);
}

bool WebViewWindow::getCookieEnabled()
{
    return m_cookieEnabled;
}

void WebViewWindow::toggleCookieEnabled()
{
    auto policy = getCookieEnabled() ? kWKHTTPCookieAcceptPolicyNever : kWKHTTPCookieAcceptPolicyAlways;
    updateCookieAcceptPolicy(policy);
}

void WebViewWindow::updateCookieAcceptPolicy(WKHTTPCookieAcceptPolicy policy)
{
    if (auto cookieStore = WKWebsiteDataStoreGetHTTPCookieStore(m_context->websiteDataStore())) {
        WKHTTPCookieStoreSetHTTPCookieAcceptPolicy(cookieStore, policy, nullptr, nullptr);
        m_cookieEnabled = (policy != kWKHTTPCookieAcceptPolicyNever);
    }
}

#if PLATFORM(PLAYSTATION) // for downstream-only
void WebViewWindow::toggleNetworkBandWidthMode()
{
    if (m_networkBandwidthMode == kWKNetworkBandwidthModeNormal)
        m_networkBandwidthMode = kWKNetworkBandwidthModePrivileged;
    else if (m_networkBandwidthMode == kWKNetworkBandwidthModePrivileged)
        m_networkBandwidthMode = kWKNetworkBandwidthModeNormal;

    WKWebsiteDataStoreSetNetworkBandwidth(m_context->websiteDataStore(), m_networkBandwidthMode);
}

bool WebViewWindow::isWebSecurityTMServiceEnabled(WebSecurityTMService service)
{
    auto currentService = WKWebsiteDataStoreGetWebSecurityTMService(m_context->websiteDataStore());
    return currentService & service;
}

void WebViewWindow::toggleWebSecurityTMService(WebSecurityTMService service)
{
    auto currentService = WKWebsiteDataStoreGetWebSecurityTMService(m_context->websiteDataStore());
    int selectedService = currentService ^ service;

    WKWebsiteDataStoreSetWebSecurityTMService(m_context->websiteDataStore(), selectedService);
}
#endif

void WebViewWindow::toggleScaleFactor()
{
    static const int scaleRange = 3;
    double scale = WKPageGetPageScaleFactor(WKViewGetPage(m_view));
    scale = ((int)scale % scaleRange) + 1.0f;

    WKPageSetPageScaleFactor(WKViewGetPage(m_view), scale);
}

void WebViewWindow::toggleZoomFactor()
{
    static const int zoomRange = 3;

    WKPageRef page = WKViewGetPage(m_view);
    double zoom = WKPageGetPageZoomFactor(page);
    zoom = ((int)zoom % zoomRange) + 1.0f;

    WKPageSetPageZoomFactor(page, zoom);
}

void WebViewWindow::toggleViewScaleFactor()
{
    static const int scaleRange = 3;
    double scale = WKPageGetViewScaleFactor(WKViewGetPage(m_view));
    scale = ((int)scale % scaleRange) + 1.0f;

    WKPageSetViewScaleFactor(WKViewGetPage(m_view), scale);
}

void WebViewWindow::setDrawsBackground(bool enable)
{
    m_drawsBackground = enable;
    WKPageSetDrawsBackground(WKViewGetPage(m_view), m_drawsBackground);
}

void WebViewWindow::deleteCookie()
{
    m_context->deleteCookie();
}

void WebViewWindow::deleteWebsiteData()
{
    m_context->deleteWebsiteData();
}

void WebViewWindow::setAcceleratedCompositing(bool enable)
{
    m_acceleratedCompositingEnabled = enable;

    WKPreferencesSetAcceleratedDrawingEnabled(m_preferences, m_acceleratedCompositingEnabled);
    WKPreferencesSetCanvasUsesAcceleratedDrawing(m_preferences, m_acceleratedCompositingEnabled);
    WKPreferencesSetAcceleratedCompositingEnabled(m_preferences, m_acceleratedCompositingEnabled);
}

void WebViewWindow::setResourceLoadStatisticsEnabled(bool enable)
{
    m_context->setResourceLoadStatisticsEnabled(enable);
}

bool WebViewWindow::getResourceLoadStatisticsEnabled()
{
    return m_context->getResourceLoadStatisticsEnabled();
}

void WebViewWindow::setCompositingBordersVisible(bool visible)
{
    m_context->setCompositingBordersVisible(visible);
}

bool WebViewWindow::getCompositingBordersVisible()
{
    return m_context->getCompositingBordersVisible();
}

void WebViewWindow::setTiledScrollingIndicatorVisible(bool visible)
{
    m_context->setTiledScrollingIndicatorVisible(visible);
}

bool WebViewWindow::getTiledScrollingIndicatorVisible()
{
    return m_context->getTiledScrollingIndicatorVisible();
}

void WebViewWindow::setLargeImageAsyncDecodingEnabled(bool enable)
{
    m_context->setLargeImageAsyncDecodingEnabled(enable);
}

bool WebViewWindow::getLargeImageAsyncDecodingEnabled()
{
    return m_context->getLargeImageAsyncDecodingEnabled();
}

void WebViewWindow::setVideoPlaybackRequiresUserGesture(bool enable)
{
    m_context->setVideoPlaybackRequiresUserGesture(enable);
}

bool WebViewWindow::getVideoPlaybackRequiresUserGesture()
{
    return m_context->getVideoPlaybackRequiresUserGesture();
}

void WebViewWindow::updateCompositingBordersVisible()
{
    bool visible = m_context->getCompositingBordersVisible();
    WKPreferencesSetCompositingBordersVisible(m_preferences, visible);
    WKPreferencesSetCompositingRepaintCountersVisible(m_preferences, visible);
}

void WebViewWindow::updateTiledScrollingIndicatorVisible()
{
    WKPreferencesSetTiledScrollingIndicatorVisible(m_preferences, m_context->getTiledScrollingIndicatorVisible());
}

void WebViewWindow::updateLargeImageAsyncDecodingEnabled()
{
    WKPreferencesSetLargeImageAsyncDecodingEnabled(m_preferences, m_context->getLargeImageAsyncDecodingEnabled());
}

void WebViewWindow::updateVideoPlaybackRequiresUserGesture()
{
    WKPreferencesSetVideoPlaybackRequiresUserGesture(m_preferences, m_context->getVideoPlaybackRequiresUserGesture());
}

void WebViewWindow::removeNetworkCache()
{
    m_context->removeNetworkCache();
}

void WebViewWindow::enterAcceleratedCompositingMode(WKViewRef, uint32_t canvasHandle, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    webView->setCanvasHandle(canvasHandle);
}

void WebViewWindow::exitAcceleratedCompositingMode(WKViewRef, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    webView->setCanvasHandle(canvas::Canvas::kInvalidHandle);
#if HAVE(OPENGL_SERVER_SUPPORT)
    webView->stopOpenGLServerIfNeeded();
#endif
}

int WebViewWindow::launchOpenGLServerProcess(WKViewRef, const void* clientInfo)
{
#if HAVE(OPENGL_SERVER_SUPPORT)
    if (WebViewWindow* webView = toWebView(clientInfo))
        return webView->openGLServerConnection();
#endif
    return 0;
}

void WebViewWindow::didChangeContentsVisibility(WKViewRef, WKSize, WKPoint, float, const void*)
{
}

void WebViewWindow::didChangeTitle(const void* clientInfo)
{
    if (WebViewWindow* webView = toWebView(clientInfo)) {
        if (auto title = WKPageCopyTitle(WKViewGetPage(webView->m_view))) {
            webView->m_currentTitle = toUTF8String(title);
            WKRelease(title);
        } else
            webView->m_currentTitle = "";

        webView->notifyViewInformation(kNotifyTitle);
    }
}

void WebViewWindow::didChangeActiveURL(const void* clientInfo)
{
    if (WebViewWindow* webView = toWebView(clientInfo)) {
        webView->m_currentURL = "";
        if (auto url = WKPageCopyActiveURL(WKViewGetPage(webView->m_view))) {
            if (auto urlString = WKURLCopyString(url)) {
                webView->m_currentURL = toUTF8String(urlString);
                WKRelease(urlString);
            }
            WKRelease(url);
        }
        webView->notifyViewInformation(kNotifyURL);
    }
}

void WebViewWindow::didChangeEstimatedProgress(const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    webView->m_currentProgress = WKPageGetEstimatedProgress(WKViewGetPage(webView->m_view));
    webView->notifyViewInformation(kNotifyProgress);
}

void WebViewWindow::didChangeWebProcessIsResponsive(const void* clientInfo)
{
    if (WebViewWindow* webView = toWebView(clientInfo))
        printf("WebProcess is %s\n", WKPageWebProcessIsResponsive(WKViewGetPage(webView->m_view)) ? "Responsive" : "Unresponsive");
}

void WebViewWindow::didSwapWebProcesses(const void*)
{
    printf("didSwapWebProcesses\n");
}

// must be same order as WebCore::AXObjectCache::AXNotification
static std::map<WKAXNotification, const char*> axNotificationMap = {
    { kWKAXAccessKeyChanged, "AccessKeyChanged" },
    { kWKAXActiveDescendantChanged, "ActiveDescendantChanged" },
    { kWKAXRoleChanged, "RoleChanged" },
    { kWKAXAutocorrectionOccured, "AutocorrectionOccured" },
    { kWKAXCellSlotsChanged, "CellSlotsChange" },
    { kWKAXCheckedStateChanged, "CheckedStateChanged" },
    { kWKAXChildrenChanged, "ChildrenChanged" },
    { kWKAXCurrentStateChanged, "CurrentStateChanged" },
    { kWKAXDisabledStateChanged, "DisabledStateChanged" },
    { kWKAXFocusedUIElementChanged, "FocusedUIElementChanged" },
    { kWKAXFrameLoadComplete, "FrameLoadComplete" },
    { kWKAXIdAttributeChanged, "IdAttributeChanged" },
    { kWKAXImageOverlayChanged, "ImageOverlayChanged" },
    { kWKAXLanguageChanged, "LanguageChanged" },
    { kWKAXLayoutComplete, "LayoutComplete" },
    { kWKAXLoadComplete, "LoadComplete" },
    { kWKAXNewDocumentLoadComplete, "NewDocumentLoadComplete" },
    { kWKAXPageScrolled, "PageScrolled" },
    { kWKAXSelectedChildrenChanged, "SelectedChildrenChanged" },
    { kWKAXSelectedStateChanged, "SelectedStateChanged" },
    { kWKAXSelectedTextChanged, "SelectedTextChanged" },
    { kWKAXValueChanged, "ValueChanged" },
    { kWKAXScrolledToAnchor, "ScrolledToAnchor" },
    { kWKAXLiveRegionCreated, "LiveRegionCreated" },
    { kWKAXLiveRegionChanged, "LiveRegionChanged" },
    { kWKAXMenuListItemSelected, "MenuListItemSelected" },
    { kWKAXMenuListValueChanged, "MenuListValueChanged" },
    { kWKAXMenuClosed, "MenuClosed" },
    { kWKAXMenuOpened, "MenuOpened" },
    { kWKAXRowCountChanged, "RowCountChanged" },
    { kWKAXRowCollapsed, "RowCollapsed" },
    { kWKAXRowExpanded, "RowExpanded" },
    { kWKAXExpandedChanged, "ExpandedChanged" },
    { kWKAXInvalidStatusChanged, "InvalidStatusChanged" },
    { kWKAXPressDidSucceed, "PressDidSucceed" },
    { kWKAXPressDidFail, "PressDidFail" },
    { kWKAXPressedStateChanged, "PressedStateChanged" },
    { kWKAXReadOnlyStatusChanged, "ReadOnlyStatusChanged" },
    { kWKAXRequiredStatusChanged, "RequiredStatusChanged" },
    { kWKAXSortDirectionChanged, "SortDirectionChanged" },
    { kWKAXTextChanged, "TextChanged" },
    { kWKAXElementBusyChanged, "ElementBusyChanged" },
    { kWKAXDraggingStarted, "DraggingStarted" },
    { kWKAXDraggingEnded, "DraggingEnded" },
    { kWKAXDraggingEnteredDropZone, "DraggingEnteredDropZone" },
    { kWKAXDraggingDropped, "DraggingDropped" },
    { kWKAXDraggingExitedDropZone, "DraggingExitedDropZone" },
};

// must be same order as WebCore::AXObjectCache::AXTextChange
static std::map<WKAXTextChange, const char*> axTextChangeMap = {
    { kWKAXTextInserted, "TextInserted" },
    { kWKAXTextDeleted, "TextDeleted" },
    { kWKAXTextAttributesChanged, "kWKAXTextAttributesChanged" },
};

// // must be same order as WebCore::AXObjectCache::AXLoadingEvent
static std::map<WKAXLoadingEvent, const char*> axLoadingEventMap = {
    { kWKAXLoadingStarted, "LoadingStarted" },
    { kWKAXLoadingReloaded, "LoadingReloaded" },
    { kWKAXLoadingFailed, "LoadingFailed" },
    { kWKAXLoadingFinished, "LoadingFinished" },
};

// must be same order as WebCore::AccessibilityRole
static std::map<WKAXRole, const char*> axRoleMap = {
    { kWKAnnotation, "AnnotationRole" },
    { kWKApplication, "ApplicationRole" },
    { kWKApplicationAlert, "ApplicationAlertRole" },
    { kWKApplicationAlertDialog, "ApplicationAlertDialogRole" },
    { kWKApplicationDialog, "ApplicationDialogRole" },
    { kWKApplicationGroup, "ApplicationGroupRole" },
    { kWKApplicationLog, "ApplicationLogRole" },
    { kWKApplicationMarquee, "ApplicationMarqueeRole" },
    { kWKApplicationStatus, "ApplicationStatusRole" },
    { kWKApplicationTextGroup, "ApplicationTextGroupRole" },
    { kWKApplicationTimer, "ApplicationTimerRole" },
    { kWKAudio, "AudioRole" },
    { kWKBlockquote, "BlockquoteRole" },
    { kWKBrowser, "BrowserRole" },
    { kWKBusyIndicator, "BusyIndicatorRole" },
    { kWKButton, "ButtonRole" },
    { kWKCanvas, "CanvasRole" },
    { kWKCaption, "CaptionRole" },
    { kWKCell, "CellRole" },
    { kWKCheckBox, "CheckBoxRole" },
    { kWKColorWell, "ColorWellRole" },
    { kWKColumn, "ColumnRole" },
    { kWKColumnHeader, "ColumnHeaderRole" },
    { kWKComboBox, "ComboBoxRole" },
    { kWKDefinition, "DefinitionRole" },
    { kWKDeletion, "DeletionRole" },
    { kWKDescriptionList, "DescriptionListRole" },
    { kWKDescriptionListTerm, "DescriptionListTermRole" },
    { kWKDescriptionListDetail, "DescriptionListDetailRole" },
    { kWKDetails, "DetailsRole" },
    { kWKDirectory, "DirectoryRole" },
    { kWKDisclosureTriangle, "DisclosureTriangleRole" },
    { kWKDiv, "DivRole" },
    { kWKDocument, "DocumentRole" },
    { kWKDocumentArticle, "DocumentArticleRole" },
    { kWKDocumentMath, "DocumentMathRole" },
    { kWKDocumentNote, "DocumentNoteRole" },
    { kWKDrawer, "DrawerRole" },
    { kWKEditableText, "EditableTextRole" },
    { kWKFeed, "FeedRole" },
    { kWKFigure, "FigureRole" },
    { kWKFooter, "FooterRole" },
    { kWKFootnote, "FootnoteRole" },
    { kWKForm, "FormRole" },
    { kWKGraphicsDocument, "GraphicsDocumentRole" },
    { kWKGraphicsObject, "GraphicsObjectRole" },
    { kWKGraphicsSymbol, "GraphicsSymbolRole" },
    { kWKGrid, "GridRole" },
    { kWKGridCell, "GridCellRole" },
    { kWKGroup, "GroupRole" },
    { kWKGrowArea, "GrowAreaRole" },
    { kWKHeading, "HeadingRole" },
    { kWKHelpTag, "HelpTagRole" },
    { kWKHorizontalRule, "HorizontalRuleRole" },
    { kWKIgnored, "IgnoredRole" },
    { kWKInline, "InlineRole" },
    { kWKImage, "ImageRole" },
    { kWKImageMap, "ImageMapRole" },
    { kWKImageMapLink, "ImageMapLinkRole" },
    { kWKIncrementor, "IncrementorRole" },
    { kWKInsertion, "InsertionRole" },
    { kWKLabel, "LabelRole" },
    { kWKLandmarkBanner, "LandmarkBannerRole" },
    { kWKLandmarkComplementary, "LandmarkComplementaryRole" },
    { kWKLandmarkContentInfo, "LandmarkContentInfoRole" },
    { kWKLandmarkDocRegion, "LandmarkDocRegionRole" },
    { kWKLandmarkMain, "LandmarkMainRole" },
    { kWKLandmarkNavigation, "LandmarkNavigationRole" },
    { kWKLandmarkRegion, "LandmarkRegionRole" },
    { kWKLandmarkSearch, "LandmarkSearchRole" },
    { kWKLegend, "LegendRole" },
    { kWKLink, "LinkRole" },
    { kWKList, "ListRole" },
    { kWKListBox, "ListBoxRole" },
    { kWKListBoxOption, "ListBoxOptionRole" },
    { kWKListItem, "ListItemRole" },
    { kWKListMarker, "ListMarkerRole" },
    { kWKMark, "MarkRole" },
    { kWKMathElement, "MathElementRole" },
    { kWKMatte, "MatteRole" },
    { kWKMenu, "MenuRole" },
    { kWKMenuBar, "MenuBarRole" },
    { kWKMenuButton, "MenuButtonRole" },
    { kWKMenuItem, "MenuItemRole" },
    { kWKMenuItemCheckbox, "MenuItemCheckboxRole" },
    { kWKMenuItemRadio, "MenuItemRadioRole" },
    { kWKMenuListPopup, "MenuListPopupRole" },
    { kWKMenuListOption, "MenuListOptionRole" },
    { kWKMeter, "MeterRole" },
    { kWKModel, "ModelRole" },
    { kWKOutline, "OutlineRole" },
    { kWKParagraph, "ParagraphRole" },
    { kWKPopUpButton, "PopUpButtonRole" },
    { kWKPre, "PreRole" },
    { kWKPresentational, "PresentationalRole" },
    { kWKProgressIndicator, "ProgressIndicatorRole" },
    { kWKRadioButton, "RadioButtonRole" },
    { kWKRadioGroup, "RadioGroupRole" },
    { kWKRowHeader, "RowHeaderRole" },
    { kWKRow, "RowRole" },
    { kWKRowGroup, "RowGroupRole" },
    { kWKRubyBase, "RubyBaseRole" },
    { kWKRubyBlock, "RubyBlockRole" },
    { kWKRubyInline, "RubyInlineRole" },
    { kWKRubyRun, "RubyRunRole" },
    { kWKRubyText, "RubyTextRole" },
    { kWKRuler, "RulerRole" },
    { kWKRulerMarker, "RulerMarkerRole" },
    { kWKScrollArea, "ScrollAreaRole" },
    { kWKScrollBar, "ScrollBarRole" },
    { kWKSearchField, "SearchFieldRole" },
    { kWKSheet, "SheetRole" },
    { kWKSlider, "SliderRole" },
    { kWKSliderThumb, "SliderThumbRole" },
    { kWKSpinButton, "SpinButtonRole" },
    { kWKSpinButtonPart, "SpinButtonPartRole" },
    { kWKSplitGroup, "SplitGroupRole" },
    { kWKSplitter, "SplitterRole" },
    { kWKStaticText, "StaticTextRole" },
    { kWKSubscript, "SubscriptRole" },
    { kWKSummary, "SummaryRole" },
    { kWKSuperscript, "SuperscriptRole" },
    { kWKSwitch, "SwitchRole" },
    { kWKSystemWide, "SystemWideRole" },
    { kWKSVGRoot, "SVGRootRole" },
    { kWKSVGText, "SVGTextRole" },
    { kWKSVGTSpan, "SVGTSpanRole" },
    { kWKSVGTextPath, "SVGTextPathRole" },
    { kWKTabGroup, "TabGroupRole" },
    { kWKTabList, "TabListRole" },
    { kWKTabPanel, "TabPanelRole" },
    { kWKTab, "TabRole" },
    { kWKTable, "TableRole" },
    { kWKTableHeaderContainer, "TableHeaderContainerRole" },
    { kWKTextArea, "TextAreaRole" },
    { kWKTextGroup, "TextGroupRole" },
    { kWKTerm, "TermRole" },
    { kWKTime, "TimeRole" },
    { kWKTree, "TreeRole" },
    { kWKTreeGrid, "TreeGridRole" },
    { kWKTreeItem, "TreeItemRole" },
    { kWKTextField, "TextFieldRole" },
    { kWKToggleButton, "ToggleButtonRole" },
    { kWKToolbar, "ToolbarRole" },
    { kWKUnknown, "UnknownRole" },
    { kWKUserInterfaceTooltip, "UserInterfaceTooltipRole" },
    { kWKValueIndicator, "ValueIndicatorRole" },
    { kWKVideo, "VideoRole" },
    { kWKWebApplication, "WebApplicationRole" },
    { kWKWebArea, "WebAreaRole" },
    { kWKWebCoreLink, "WebCoreLinkRole" },
    { kWKWindow, "WindowRole" },
};

#if ENABLE(ACCESSIBILITY)
void WebViewWindow::setAccessibilityEnabled(bool enable)
{
    if (m_accessibilityEnabled == enable)
        return;

    m_accessibilityEnabled = enable;
    WKContextSetAccessibilityEnabled(m_context->context(), enable);
}

bool WebViewWindow::getAccessibilityEnabled()
{
    bool enable = WKContextGetAccessibilityEnabled(m_context->context());
    ASSERT(enable == m_accessibilityEnabled);
    return enable;
}

void WebViewWindow::borderAccessibilityObject(WKAXObjectRef axObject, const void* clientInfo)
{
    if (WebViewWindow* webView = toWebView(clientInfo)) {
        WKAXRole role = WKAXObjectRole(axObject);
        bool isFocusShow = false;
        switch (role) {
        case kWKApplicationAlert:
        case kWKApplicationAlertDialog:
        case kWKApplicationDialog:
        case kWKButton:
        case kWKCheckBox:
        case kWKColorWell:
        case kWKComboBox:
        case kWKHeading:
        case kWKImage:
        case kWKImageMapLink:
        case kWKListBoxOption:
        case kWKPopUpButton:
        case kWKRadioButton:
        case kWKStaticText:
        case kWKTextArea:
        case kWKTextField:
        case kWKToggleButton:
        case kWKWebCoreLink:
            isFocusShow = true;
            break;
        }

        if (isFocusShow) {
            IntRect rect = toTKRect(WKRectGetValue(WKAXObjectRect(axObject)));
            webView->m_axFocusWidget->setPosition(rect.position);
            webView->m_axFocusWidget->setSize(rect.size);
        } else {
            webView->m_axFocusWidget->setPosition(IntPoint().zero());
            webView->m_axFocusWidget->setSize(IntSize().zero());
        }
    }
}

void WebViewWindow::dumpAccessibilityObject(WKAXObjectRef axObject)
{
    WKAXRole role = WKAXObjectRole(axObject);

    auto title = adoptWK(WKAXObjectCopyTitle(axObject));
    auto description = adoptWK(WKAXObjectCopyDescription(axObject));
    auto helpText = adoptWK(WKAXObjectCopyHelpText(axObject));
    IntRect rect = toTKRect(WKRectGetValue(WKAXObjectRect(axObject)));
    auto value = adoptWK(WKAXObjectCopyValue(axObject));
    bool isFocused = WKBooleanGetValue((WKAXObjectIsFocused(axObject)));
    bool isDisabled = WKBooleanGetValue((WKAXObjectIsDisabled(axObject)));
    bool isSelected = WKBooleanGetValue((WKAXObjectIsSelected(axObject)));
    bool isVisited = WKBooleanGetValue((WKAXObjectIsVisited(axObject)));
    auto buttonState = WKAXObjectButtonState(axObject);

    string buttonStateStr = "";
    switch (buttonState) {
    case 0:
        buttonStateStr = "Off";
        break;
    case 1:
        buttonStateStr = "On";
        break;
    case 2:
        buttonStateStr = "Mixed";
        break;
    default:
        break;
    }

    printf("role:%s, title:%s, description:%s, helpText:%s, val:%s, check: %s, focus:%s, disable:%s, select:%s, visited:%s, rect: x:%d y:%d w:%d h:%d \n",
        axRoleMap.at(role),
        toUTF8String(title.get()).c_str(),
        toUTF8String(description.get()).c_str(),
        toUTF8String(helpText.get()).c_str(),
        toUTF8String(value.get()).c_str(),
        buttonStateStr.c_str(),
        isFocused ? "Yes" : "No",
        isDisabled ? "Yes" : "No",
        isSelected ? "Yes" : "No",
        isVisited ? "Yes" : "No",
        rect.left(),
        rect.top(),
        rect.width(),
        rect.height());
}

void WebViewWindow::accessibilityNotification(WKViewRef, WKAXObjectRef axObject, WKAXNotification notification, const void*)
{
    printf("[AX Notification %s] ", axNotificationMap[notification]);
    dumpAccessibilityObject(axObject);
}

void WebViewWindow::accessibilityTextChanged(WKViewRef, WKAXObjectRef, WKAXTextChange textChange, uint32_t offset, WKStringRef text, const void*)
{
    printf("[AX TextChanged %s], text:%s, offset:%d ", axTextChangeMap[textChange], toUTF8String(text).c_str(), offset);
}

void WebViewWindow::accessibilityLoadingEvent(WKViewRef, WKAXObjectRef axObject, WKAXLoadingEvent loadingEvent, const void*)
{
    auto url = adoptWK(WKAXObjectCopyURL(axObject));
    printf("[AX LoadingEvent %s] url: %s, ", axLoadingEventMap[loadingEvent], toUTF8String(url.get()).c_str());
    dumpAccessibilityObject(axObject);
}

void WebViewWindow::accessibilityRootObject(WKViewRef, WKAXObjectRef axObject, const void*)
{
    printf("[AX RootObject] ");
    dumpAccessibilityObject(axObject);
}

void WebViewWindow::accessibilityFocusedObject(WKViewRef, WKAXObjectRef axObject, const void*)
{
    printf("[AX FocusedObject] ");
    dumpAccessibilityObject(axObject);
}

void WebViewWindow::accessibilityHitTest(WKViewRef, WKAXObjectRef axObject, const void* clientInfo)
{
    printf("[AX HitTest] ");
    dumpAccessibilityObject(axObject);
    borderAccessibilityObject(axObject, clientInfo);
}
#endif

void WebViewWindow::showPopupMenu(WKViewRef view, WKArrayRef popupMenuItem, WKRect, int32_t selectedIndex, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    if (webView->m_popupMenu)
        webView->removeChild(webView->m_popupMenu);

    size_t size = WKArrayGetSize(popupMenuItem);
    if (!size)
        return;

    auto menu = make_unique<Menu>();
    for (size_t i = 0; i < size; i++) {
        WKTypeRef item = WKArrayGetItemAtIndex(popupMenuItem, i);
        WKPopupMenuItemRef popupItem = static_cast<WKPopupMenuItemRef>(item);

        bool isItem = WKPopupMenuItemIsItem(popupItem);
        bool isSeparator = WKPopupMenuItemIsSeparator(popupItem);
        bool isEnabled = WKPopupMenuItemIsEnabled(popupItem);
        auto label = toUTF8String(adoptWK(WKPopupMenuItemCopyText(popupItem)).get());

        if (isSeparator)
            label = "---" + label;
        else if (!isEnabled)
            label = "   " + label;
        else if (static_cast<int32_t>(i) == selectedIndex)
            label = "[*]" + label;
        else
            label = "[ ]" + label;

        menu->addItem(label.c_str(), [=](Menu*, IntRect) {
            if (!isSeparator && isItem && isEnabled) {
                WKViewValueChangedForPopupMenu(view, i);
                hidePopupMenu(view, clientInfo);
            }
        });
    }

    webView->m_popupMenu = menu.get();
    webView->appendChild(std::move(menu));
}

void WebViewWindow::hidePopupMenu(WKViewRef, const void* clientInfo)
{
    WebViewWindow* webView = toWebView(clientInfo);
    if (webView->m_popupMenu)
        webView->removeChild(webView->m_popupMenu);
}

void WebViewWindow::addPunchHoleWidget()
{
    auto widget = std::make_unique<PunchHoleWidget>();
    m_punchHoles.push_back(widget.get());
    appendChild(std::move(widget));
}

void WebViewWindow::removePunchHoleWidget()
{
    ASSERT(m_punchHoles.size() >= 1);
    auto removing = m_punchHoles.back();
    m_punchHoles.pop_back();
    removeChild(removing);
}

void WebViewWindow::updatePunchHoles(WKPunchHole* holes, int numberOfHoles)
{
    // FIXME: We should not calculate length each iterates.
    // Discard widgets if we have too many punchhole widget.
    while (static_cast<int>(m_punchHoles.size()) > numberOfHoles)
        removePunchHoleWidget();

    // Create widget if we don't have enough.
    while (static_cast<int>(m_punchHoles.size()) < numberOfHoles)
        addPunchHoleWidget();

    // Update punchholes.
    ASSERT(static_cast<int>(m_punchHoles.size()) == numberOfHoles);

    for (auto punchHoleWidget : m_punchHoles) {
        punchHoleWidget->activateAs(*holes);
        holes++;
    }
}

#if HAVE(OPENGL_SERVER_SUPPORT)
int WebViewWindow::openGLServerConnection()
{
    launchOpenGLServerIfNeeded();
    return m_openGLServerProcessProxy->serverConnection();
}

void WebViewWindow::launchOpenGLServerIfNeeded()
{
    if (!m_openGLServerProcessProxy)
        m_openGLServerProcessProxy = OpenGLServerProcessProxy::create();
}

void WebViewWindow::stopOpenGLServerIfNeeded()
{
    if (m_openGLServerProcessProxy)
        m_openGLServerProcessProxy = nullptr;
}
#endif

void WebViewWindow::startWheelEventCapture()
{
    m_lastWheelEventCaptureTime = chrono::system_clock::now();
}

void WebViewWindow::setReloadWhenWebProcessDidTerminate(bool reloadWhenWebProcessDidTerminate)
{
    m_reloadWhenWebProcessDidTerminate = reloadWhenWebProcessDidTerminate;
}
