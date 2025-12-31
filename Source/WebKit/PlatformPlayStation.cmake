include(Headers.cmake)

set(WebKit_USE_PREFIX_HEADER ON)

WEBKIT_ADD_TARGET_CXX_FLAGS(WebKit -Wno-unused-lambda-capture)

list(APPEND WebProcess_SOURCES
    WebProcess/EntryPoint/playstation/WebProcessMain.cpp
)
list(APPEND WebProcess_PRIVATE_LIBRARIES
    ${EGL_LIBRARIES}
    ${ProcessLauncher_LIBRARY}
    OpenSSL::Crypto
)

list(APPEND NetworkProcess_SOURCES
    NetworkProcess/EntryPoint/playstation/NetworkProcessMain.cpp
)
list(APPEND NetworkProcess_PRIVATE_LIBRARIES
    ${ProcessLauncher_LIBRARY}
    OpenSSL::Crypto
)

list(APPEND GPUProcess_SOURCES
    GPUProcess/EntryPoint/playstation/GPUProcessMain.cpp
)
list(APPEND GPUProcess_PRIVATE_LIBRARIES
    ${ProcessLauncher_LIBRARY}
    ${EGL_LIBRARIES}
)

list(APPEND WebKit_SOURCES
    GPUProcess/media/playstation/RemoteMediaPlayerProxyPlayStation.cpp

    GPUProcess/playstation/GPUProcessMainPlayStation.cpp
    GPUProcess/playstation/GPUProcessPlayStation.cpp

    NetworkProcess/NetworkDataTaskDataURL.cpp

    NetworkProcess/Classifier/WebResourceLoadStatisticsStore.cpp

    NetworkProcess/Cookies/curl/WebCookieManagerCurl.cpp

    NetworkProcess/Cookies/playstation/WebCookieManagerPlayStation.cpp

    NetworkProcess/cache/NetworkCacheDataCurl.cpp
    NetworkProcess/cache/NetworkCacheIOChannelCurl.cpp

    NetworkProcess/curl/NetworkDataTaskCurl.cpp
    NetworkProcess/curl/NetworkProcessCurl.cpp
    NetworkProcess/curl/NetworkProcessMainCurl.cpp
    NetworkProcess/curl/NetworkSessionCurl.cpp
    NetworkProcess/curl/WebSocketTaskCurl.cpp

    Platform/IPC/unix/ArgumentCodersUnix.cpp
    Platform/IPC/unix/ConnectionUnix.cpp
    Platform/IPC/unix/IPCSemaphoreUnix.cpp

    Platform/classifier/ResourceLoadStatisticsClassifier.cpp

    Platform/unix/LoggingUnix.cpp
    Platform/unix/ModuleUnix.cpp
    Platform/unix/SharedMemoryUnix.cpp

    Shared/API/c/cairo/WKImageCairo.cpp

    Shared/API/c/curl/WKCertificateInfoCurl.cpp

    Shared/API/c/neko/WKEventPlayStation.cpp
    Shared/API/c/neko/WKRegistrableDomainPlayStation.cpp

#    Shared/API/c/playstation/WKEventPlayStation.cpp

    Shared/Plugins/Netscape/NetscapePluginModuleNone.cpp

    Shared/cairo/ShareableBitmapCairo.cpp

    Shared/freetype/WebCoreArgumentCodersFreeType.cpp

    Shared/curl/WebCoreArgumentCodersCurl.cpp

#    Shared/libwpe/NativeWebKeyboardEventLibWPE.cpp
    Shared/libwpe/NativeWebMouseEventLibWPE.cpp
    Shared/libwpe/NativeWebTouchEventLibWPE.cpp
    Shared/libwpe/NativeWebWheelEventLibWPE.cpp
#    Shared/libwpe/WebEventFactory.cpp

    Shared/neko/AssertReportPlayStation.cpp
    Shared/neko/AuxiliaryProcessMain.cpp
    Shared/neko/EnvVarUtils.cpp
    Shared/neko/NativeWebKeyboardEventPlayStation.cpp
    Shared/neko/WebAccessibilityObject.cpp
    Shared/neko/WebAccessibilityObjectData.cpp
    Shared/neko/WebEventFactory.cpp
    Shared/neko/WebPunchHole.cpp

#    Shared/playstation/WebCoreArgumentCodersPlayStation.cpp

#    Shared/unix/AuxiliaryProcessMain.cpp

    UIProcess/BackingStore.cpp
    UIProcess/DefaultUndoController.cpp
    UIProcess/LegacySessionStateCodingNone.cpp
    UIProcess/WebGrammarDetail.cpp
    UIProcess/WebMemoryPressureHandler.cpp
    UIProcess/WebViewportAttributes.cpp

    UIProcess/API/C/WKViewportAttributes.cpp

    UIProcess/API/C/WKUserScriptRef.cpp

    UIProcess/API/C/curl/WKProtectionSpaceCurl.cpp
    UIProcess/API/C/curl/WKWebsiteDataStoreRefCurl.cpp

    UIProcess/API/C/neko/WKAuthenticationChallengePlayStation.cpp
    UIProcess/API/C/neko/WKAXObject.cpp
    UIProcess/API/C/neko/WKContextConfigurationPlayStation.cpp
    UIProcess/API/C/neko/WKContextPlayStation.cpp
    UIProcess/API/C/neko/WKHTTPCookieStoreRefPlayStation.cpp
    UIProcess/API/C/neko/WKImeEvent.cpp
    UIProcess/API/C/neko/WKNavigationActionRefPlayStation.cpp
    UIProcess/API/C/neko/WKNavigationResponseRefPlayStation.cpp
    UIProcess/API/C/neko/WKPagePrivatePlayStation.cpp
    UIProcess/API/C/neko/WKPopupMenuItem.cpp
    UIProcess/API/C/neko/WKPreferencesPlayStation.cpp
    UIProcess/API/C/neko/WKRunLoop.cpp
    UIProcess/API/C/neko/WKView.cpp
    UIProcess/API/C/neko/WKWebsiteDataStoreRefPlayStation.cpp

#    UIProcess/API/C/playstation/WKContextConfigurationPlayStation.cpp
#    UIProcess/API/C/playstation/WKPagePrivatePlayStation.cpp
#    UIProcess/API/C/playstation/WKRunloop.cpp
#    UIProcess/API/C/playstation/WKView.cpp

    UIProcess/API/neko/APIHTTPCookieStorePlayStation.cpp
    UIProcess/API/neko/WebPopupMenuItem.cpp
    UIProcess/API/neko/WebPopupMenuProxyPlayStation.cpp

    UIProcess/Automation/cairo/WebAutomationSessionCairo.cpp

    UIProcess/Automation/playstation/WebAutomationSessionPlayStation.cpp

    UIProcess/CoordinatedGraphics/DrawingAreaProxyCoordinatedGraphics.cpp

    UIProcess/Launcher/playstation/ProcessLauncherPlayStation.cpp

    UIProcess/TextChecker.cpp

    UIProcess/WebsiteData/curl/WebsiteDataStoreCurl.cpp

    UIProcess/WebsiteData/playstation/WebsiteDataStorePlayStation.cpp

    UIProcess/cairo/BackingStoreCairo.cpp

#    UIProcess/libwpe/WebPasteboardProxyLibWPE.cpp

    UIProcess/neko/AssertReportProxy.cpp
    UIProcess/neko/AutomationClientPlayStation.cpp
    UIProcess/neko/PageClientImpl.cpp
    UIProcess/neko/PlayStationWebView.cpp
    UIProcess/neko/WebPageProxyPlayStation.cpp
    UIProcess/neko/WebPasteboardProxyPlayStation.cpp
    UIProcess/neko/WebPreferencesPlayStation.cpp
    UIProcess/neko/WebProcessPoolPlayStation.cpp
    UIProcess/neko/WebViewAccessibilityClient.cpp

#    UIProcess/playstation/PageClientImpl.cpp
#    UIProcess/playstation/PlayStationWebView.cpp
#    UIProcess/playstation/WebPageProxyPlayStation.cpp
#    UIProcess/playstation/WebProcessPoolPlayStation.cpp

    WebProcess/GPU/media/playstation/VideoLayerRemotePlayStation.cpp

    WebProcess/InjectedBundle/playstation/InjectedBundlePlayStation.cpp

    WebProcess/MediaCache/WebMediaKeyStorageManager.cpp

    WebProcess/WebCoreSupport/curl/WebFrameNetworkingContext.cpp

    WebProcess/WebCoreSupport/WebContextMenuClient.cpp
    WebProcess/WebCoreSupport/WebPopupMenu.cpp

    WebProcess/WebPage/AcceleratedSurface.cpp

    WebProcess/WebPage/CoordinatedGraphics/CompositingCoordinator.cpp
    WebProcess/WebPage/CoordinatedGraphics/DrawingAreaCoordinatedGraphics.cpp

#    WebProcess/WebPage/libwpe/AcceleratedSurfaceLibWPE.cpp

    WebProcess/WebPage/neko/AcceleratedSurfacePlayStation.cpp
    WebProcess/WebPage/neko/WebPagePlayStation.cpp

#    WebProcess/WebPage/playstation/WebPagePlayStation.cpp

    WebProcess/playstation/WebProcessMainPlayStation.cpp
    WebProcess/playstation/WebProcessPlayStation.cpp
)

add_custom_command(
    OUTPUT ${WebKit_DERIVED_SOURCES_DIR}/WebKitVersion.h
    MAIN_DEPENDENCY ${WEBKITLEGACY_DIR}/scripts/generate-webkitversion.pl
    COMMAND ${PERL_EXECUTABLE} ${WEBKITLEGACY_DIR}/scripts/generate-webkitversion.pl --config ${WEBKITLEGACY_DIR}/../../Configurations/Version.xcconfig --outputDir ${WebKit_DERIVED_SOURCES_DIR}
    VERBATIM)
list(APPEND WebKit_SOURCES ${WebKit_DERIVED_SOURCES_DIR}/WebKitVersion.h)

list(APPEND WebProcess_INCLUDE_DIRECTORIES
    ${OPENSSL_INCLUDE_DIR}
)

set(PLAYSTATION_WebProcess_AVOID_LIBC_SETUP ON)
set(PLAYSTATION_WebProcess_REQUIRED_SHARED_LIBRARIES
    ${GL_SHARED_LIBRARY_NAME}
    libSceNKWebKitRequirements
    libcairo
    libfontconfig
    libfreetype
    libharfbuzz
    libicu
    libpng16
)

list(APPEND NetworkProcess_INCLUDE_DIRECTORIES
    ${OPENSSL_INCLUDE_DIR}
)
set(PLAYSTATION_NetworkProcess_AVOID_LIBC_SETUP ON)
set(PLAYSTATION_NetworkProcess_REQUIRED_SHARED_LIBRARIES
    libSceNKWebKitRequirements
    libcurl
)

list(APPEND WebKit_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBKIT_DIR}/NetworkProcess/curl"
    "${WEBKIT_DIR}/Platform/IPC/unix"
    "${WEBKIT_DIR}/Platform/classifier"
    "${WEBKIT_DIR}/Platform/generic"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics"
    "${WEBKIT_DIR}/Shared/CoordinatedGraphics/threadedcompositor"
#    "${WEBKIT_DIR}/Shared/libwpe"
    "${WEBKIT_DIR}/Shared/neko"
    "${WEBKIT_DIR}/UIProcess/API/C/curl"
    "${WEBKIT_DIR}/UIProcess/API/C/neko"
#    "${WEBKIT_DIR}/UIProcess/API/C/playstation"
    "${WEBKIT_DIR}/UIProcess/API/libwpe"
    "${WEBKIT_DIR}/UIProcess/API/neko"
#    "${WEBKIT_DIR}/UIProcess/API/playstation"
    "${WEBKIT_DIR}/UIProcess/CoordinatedGraphics"
    "${WEBKIT_DIR}/UIProcess/neko"
#    "${WEBKIT_DIR}/UIProcess/playstation"
    "${WEBKIT_DIR}/WebProcess/WebCoreSupport/curl"
    "${WEBKIT_DIR}/WebProcess/WebPage/CoordinatedGraphics"
#    "${WEBKIT_DIR}/WebProcess/WebPage/libwpe"
    "${WEBKIT_DIR}/WebProcess/WebPage/neko"
)

list(APPEND WebKit_SYSTEM_INCLUDE_DIRECTORIES
    ${CAIRO_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
)

if (ENABLE_MAT)
    list(APPEND WebKit_PRIVATE_LIBRARIES libmat::mat)

    list(APPEND NetworkProcess_PRIVATE_LIBRARIES libmat::mat)
    list(APPEND WebProcess_PRIVATE_LIBRARIES libmat::mat)
endif ()

if (ENABLE_GAMEPAD)
    list(APPEND WebKit_SOURCES
        UIProcess/Gamepad/libwpe/UIGamepadProviderLibWPE.cpp
    )
endif ()

if (ENABLE_WEBDRIVER AND USE_WPE_BACKEND_PLAYSTATION)
    list(APPEND WebKit_SOURCES
        UIProcess/Automation/libwpe/WebAutomationSessionLibWPE.cpp
    )

    list(REMOVE_ITEM WebKit_SOURCES
        UIProcess/Automation/playstation/WebAutomationSessionPlayStation.cpp
    )
endif ()

if (USE_COORDINATED_GRAPHICS)
    list(APPEND WebKit_SOURCES
        Shared/CoordinatedGraphics/CoordinatedGraphicsScene.cpp
        Shared/CoordinatedGraphics/SimpleViewportController.cpp

        Shared/CoordinatedGraphics/threadedcompositor/CompositingRunLoop.cpp
        Shared/CoordinatedGraphics/threadedcompositor/ThreadedCompositor.cpp
        Shared/CoordinatedGraphics/threadedcompositor/ThreadedDisplayRefreshMonitor.cpp

        WebProcess/WebPage/CoordinatedGraphics/LayerTreeHost.cpp
    )
endif ()

if (USE_GRAPHICS_LAYER_WC)
    list(APPEND WebKit_SOURCES
        GPUProcess/graphics/RemoteGraphicsContextGLWC.cpp

        GPUProcess/graphics/wc/RemoteWCLayerTreeHost.cpp
        GPUProcess/graphics/wc/WCContentBufferManager.cpp
        GPUProcess/graphics/wc/WCRemoteFrameHostLayerManager.cpp
        GPUProcess/graphics/wc/WCScene.cpp
        GPUProcess/graphics/wc/WCSceneContext.cpp

        UIProcess/wc/DrawingAreaProxyWC.cpp

        WebProcess/GPU/graphics/wc/RemoteGraphicsContextGLProxyWC.cpp
        WebProcess/GPU/graphics/wc/RemoteWCLayerTreeHostProxy.cpp

        WebProcess/WebPage/CoordinatedGraphics/LayerTreeHostTextureMapper.cpp

        WebProcess/WebPage/wc/DrawingAreaWC.cpp
        WebProcess/WebPage/wc/GraphicsLayerWC.cpp
        WebProcess/WebPage/wc/WCLayerFactory.cpp
        WebProcess/WebPage/wc/WCTileGrid.cpp
    )

    list(APPEND WebKit_INCLUDE_DIRECTORIES
        "${WEBKIT_DIR}/GPUProcess/graphics/wc"
        "${WEBKIT_DIR}/Shared/wc"
        "${WEBKIT_DIR}/UIProcess/wc"
        "${WEBKIT_DIR}/WebProcess/GPU/graphics/wc"
        "${WEBKIT_DIR}/WebProcess/WebPage/wc"
    )

    list(APPEND WebKit_MESSAGES_IN_FILES
        GPUProcess/graphics/wc/RemoteWCLayerTreeHost
    )

    list(APPEND WebKit_SERIALIZATION_IN_FILES
        WebProcess/WebPage/wc/WCUpdateInfo.serialization.in
    )
endif ()

if (USE_WPE_BACKEND_PLAYSTATION)
    list(APPEND WebKit_SOURCE
        UIProcess/API/libwpe/TouchGestureController.cpp

        UIProcess/Launcher/libwpe/ProcessProviderLibWPE.cpp

        UIProcess/Launcher/playstation/ProcessProviderPlayStation.cpp
    )
    list(APPEND WebKit_INCLUDE_DIRECTORIES "${WEBKIT_DIR}/UIProcess/Launcher/libwpe")
endif ()

# PlayStation specific
list(APPEND WebKit_PUBLIC_FRAMEWORK_HEADERS
    Shared/API/c/cairo/WKImageCairo.h

    Shared/API/c/curl/WKCertificateInfoCurl.h

    Shared/API/c/neko/WKBasePlayStation.h
    Shared/API/c/neko/WKErrorRefPlayStation.h
    Shared/API/c/neko/WKEventPlayStation.h
    Shared/API/c/neko/WKRegistrableDomainPlayStation.h

#    Shared/API/c/playstation/WKBasePlayStation.h
#    Shared/API/c/playstation/WKEventPlayStation.h

    UIProcess/API/c/WKKeyValueStorageManager.h

    UIProcess/API/C/curl/WKProtectionSpaceCurl.h
    UIProcess/API/C/curl/WKWebsiteDataStoreRefCurl.h

    UIProcess/API/c/neko/WKAPICastPlayStation.h
    UIProcess/API/c/neko/WKAuthenticationChallengePlayStation.h
    UIProcess/API/c/neko/WKAXObject.h
    UIProcess/API/c/neko/WKContextConfigurationPlayStation.h
    UIProcess/API/c/neko/WKContextPlayStation.h
    UIProcess/API/c/neko/WKHTTPCookieStoreRefPlayStation.h
    UIProcess/API/c/neko/WKImeEvent.h
    UIProcess/API/c/neko/WKNavigationActionRefPlayStation.h
    UIProcess/API/c/neko/WKNavigationResponseRefPlayStation.h
    UIProcess/API/c/neko/WKPagePrivatePlayStation.h
    UIProcess/API/c/neko/WKPopupMenuItem.h
    UIProcess/API/c/neko/WKPreferencesPlayStation.h
    UIProcess/API/c/neko/WKRunLoop.h
    UIProcess/API/c/neko/WKView.h
    UIProcess/API/c/neko/WKViewAccessibilityClient.h
    UIProcess/API/c/neko/WKViewClient.h
    UIProcess/API/c/neko/WKViewPopupMenuClient.h
    UIProcess/API/c/neko/WKWebsiteDataStoreRefPlayStation.h

#    UIProcess/API/C/playstation/WKContextConfigurationPlayStation.h
#    UIProcess/API/C/playstation/WKPagePrivatePlayStation.h
#    UIProcess/API/C/playstation/WKRunloop.h
#    UIProcess/API/C/playstation/WKView.h
#    UIProcess/API/C/playstation/WKViewClient.h
)

file(GLOB WEBKIT_SHARED_HEADER_FILES "${WEBKIT_DIR}/Shared/API/C/WK*.h")
file(GLOB WEBKIT_SHARED_GENERIC_HEADER_FILES "${WEBKIT_DIR}/Shared/API/C/generic/WK*.h")
file(GLOB WEBKIT_SHARED_CURL_HEADER_FILES "${WEBKIT_DIR}/Shared/API/C/curl/WK*.h")
file(GLOB WEBKIT_SHARED_PLAYSTATION_HEADER_FILES "${WEBKIT_DIR}/Shared/API/C/neko/WK*.h")

install(FILES
    UIProcess/API/cpp/WKRetainPtr.h
    ${WebKit_DERIVED_SOURCES_DIR}/WebKitVersion.h
    ${WebKit_PUBLIC_FRAMEWORK_HEADERS}
    ${WEBKIT_INJECTEDBUNDLE_PLAYSTATION_HEADER_FILES}
    ${WEBKIT_SHARED_HEADER_FILES}
    ${WEBKIT_SHARED_GENERIC_HEADER_FILES}
    ${WEBKIT_SHARED_CURL_HEADER_FILES}
    ${WEBKIT_SHARED_PLAYSTATION_HEADER_FILES}
    DESTINATION "${WEBKITPLAYSTATION_HEADER_INSTALL_DIR}/WebKit"
)

file(GLOB INSTALL_WEBKITLIBRARIES "${WEBKIT_LIBRARIES_DIR}/lib/*.a")
install(FILES
    ${INSTALL_WEBKITLIBRARIES}
    DESTINATION ${LIB_INSTALL_DIR}
)

file(GLOB INSTALL_WEBKITLIBRARIES_PRX "${WEBKIT_LIBRARIES_DIR}/bin/*.prx")
install(FILES
    ${INSTALL_WEBKITLIBRARIES_PRX}
    DESTINATION ${LIBEXEC_INSTALL_DIR}
)

install(DIRECTORY
    "${WEBKIT_LIBRARIES_DIR}/include/"
    DESTINATION ${WEBKITPLAYSTATION_HEADER_INSTALL_DIR}
)

list(APPEND WebKit_MESSAGES_IN_FILES
    UIProcess/neko/AssertReportProxy
)
