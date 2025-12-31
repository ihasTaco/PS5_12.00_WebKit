set(TESTWEBKITAPI_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestWebKitAPI")

set(test_main_SOURCES
    playstation/main.cpp
)

list(APPEND TestWTF_SOURCES
    ${test_main_SOURCES}
)
list(APPEND TestWTF_PRIVATE_INCLUDE_DIRECTORIES
    ${WEBKIT_LIBRARIES_DIR}/include
)

list(APPEND TestWTF_LIBRARIES
    ${MEMORY_EXTRA_LIB}
)

WEBKIT_ADD_TARGET_CXX_FLAGS(TestWTF -Wno-unused-function)

list(APPEND TestJavaScriptCore_SOURCES
    ${test_main_SOURCES}
)
list(APPEND TestJavaScriptCore_PRIVATE_INCLUDE_DIRECTORIES
    ${WEBKIT_LIBRARIES_DIR}/include
)
list(APPEND TestJavaScriptCore_LIBRARIES
    ${MEMORY_EXTRA_LIB}
)

WEBKIT_ADD_TARGET_CXX_FLAGS(TestJavaScriptCore -Wno-unused-function)

list(APPEND TestWebCore_SOURCES
    ${test_main_SOURCES}
    Tests/WebCore/curl/CurlMultipartHandleTests.cpp
    Tests/WebCore/curl/OpenSSLHelperTests.cpp
)
list(APPEND TestWebCore_PRIVATE_INCLUDE_DIRECTORIES
    ${WEBKIT_LIBRARIES_DIR}/include
)

# TestWebKit
if (ENABLE_WEBKIT)
    target_sources(TestWebKitAPIInjectedBundle PRIVATE
        playstation/PlatformUtilitiesPlayStation.cpp
    )

    list(APPEND TestWebKit_SOURCES
        ${test_main_SOURCES}

        Tests/WebKit/CloseFromWithinCreatePage.cpp
        Tests/WebKit/CloseThenTerminate.cpp
        Tests/WebKit/EphemeralSessionPushStateNoHistoryCallback.cpp
        Tests/WebKit/EventModifiers.cpp
        Tests/WebKit/LayoutMilestonesWithAllContentInFrame.cpp
        Tests/WebKit/ProcessDidTerminate.cpp
        Tests/WebKit/ResizeReversePaginatedWebView.cpp
        Tests/WebKit/ShouldKeepCurrentBackForwardListItemInList.cpp
        Tests/WebKit/TerminateTwice.cpp
        Tests/WebKit/WKPageConfiguration.cpp

        Tests/WebKit/neko/BadSsl.cpp

        Tests/WebKit/playstation/Cursor.cpp
        Tests/WebKit/playstation/FullScreen.cpp
        Tests/WebKit/playstation/PopupMenu.cpp

        playstation/PlatformUtilitiesPlayStation.cpp
        playstation/PlatformWebViewPlayStation.cpp
    )
    list(APPEND TestWebKit_PRIVATE_INCLUDE_DIRECTORIES
        ${WEBKIT_LIBRARIES_DIR}/include
        # FIXME: Workaround for 262028@main. It seems better to try to insulate
        # this code to something that isn't using internals if we can.
        $<TARGET_PROPERTY:WebKit,INCLUDE_DIRECTORIES>
    )

    # Exclude tests which don't finish.
    list(REMOVE_ITEM TestWebKit_SOURCES
        Tests/WebKit/ForceRepaint.cpp
        Tests/WebKit/Geolocation.cpp
    )

    list(APPEND TestWebKit_PRIVATE_LIBRARIES
        ${ProcessLauncher_LIBRARY}
    )

    WEBKIT_ADD_TARGET_CXX_FLAGS(TestWebKit -Wno-deprecated-declarations)
endif ()

# TODO: Need to remove process-initialization headers
set(PLAYSTATION_TestWTF_AVOID_LIBC_SETUP ON)
set(PLAYSTATION_TestWebCore_AVOID_LIBC_SETUP ON)
set(PLAYSTATION_TestWebKit_AVOID_LIBC_SETUP ON)

# Supress warnings
if (COMPILER_IS_GCC_OR_CLANG)
    if (ENABLE_WEBKIT)
        WEBKIT_ADD_TARGET_CXX_FLAGS(TestWebKit -Wno-sign-compare
                                               -Wno-undef
                                               -Wno-unused-parameter)
    endif ()

    if (ENABLE_WEBCORE)
        WEBKIT_ADD_TARGET_CXX_FLAGS(TestWebCore -Wno-sign-compare
                                                -Wno-undef
                                                -Wno-unused-parameter)
    endif ()
endif ()

# Set the debugger working directory for Visual Studio
if (${CMAKE_GENERATOR} MATCHES "Visual Studio")
    set_target_properties(TestWTF PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    if (ENABLE_JAVASCRIPTCORE)
        set_target_properties(TestJavaScriptCore PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif ()
    if (ENABLE_WEBCORE)
        set_target_properties(TestWebCore PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif ()
    if (ENABLE_WEBKIT)
        set_target_properties(TestWebKit PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif ()
endif ()

add_definitions(
    -DTEST_WEBKIT_RESOURCES_DIR=\"${TOOLS_DIR}/TestWebKitAPI/Tests/WebKit\"
)

# FIXME: Workaround for 262028@main. It seems better to try to insulate
# this code to something that isn't using internals if we can.
if(TARGET TestWebKitAPIInjectedBundle)
 target_include_directories(TestWebKitAPIInjectedBundle PRIVATE $<TARGET_PROPERTY:WebKit,INCLUDE_DIRECTORIES>)
endif()
