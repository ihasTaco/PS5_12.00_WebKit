list(REMOVE_ITEM WebKitTestRunner_SOURCES
    PixelDumpSupport.cpp
)

list(APPEND WebKitTestRunner_SOURCES
    cairo/TestInvocationCairo.cpp

    neko/EventSenderProxyPlayStation.cpp
    neko/PixelDumpSupportPlayStation.cpp
    neko/PlatformWebViewPlayStation.cpp
    neko/TestControllerPlayStation.cpp
    neko/UIScriptControllerPlayStation.cpp
    neko/main.cpp
)

list(APPEND WebKitTestRunner_LIBRARIES
    ${KERNEL_LIBRARY}
    Cairo::Cairo
)

list(APPEND TestRunnerInjectedBundle_SOURCES
    InjectedBundle/neko/AccessibilityControllerPlayStation.cpp
    InjectedBundle/neko/AccessibilityUIElementPlayStation.cpp
    InjectedBundle/neko/ActivateFontsPlayStation.cpp
    InjectedBundle/neko/InjectedBundlePlayStation.cpp
    InjectedBundle/neko/TestRunnerPlayStation.cpp
)

list(APPEND TestRunnerInjectedBundle_LIBRARIES
    ${KERNEL_LIBRARY}
)

set(PLAYSTATION_WebKitTestRunner_AVOID_LIBC_SETUP ON)
