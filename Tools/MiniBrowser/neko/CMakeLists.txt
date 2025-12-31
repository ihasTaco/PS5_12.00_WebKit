set(MINIBROWSER_DIR "${TOOLS_DIR}/MiniBrowser/neko")

set(MiniBrowser_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBKIT_LIBRARIES_DIR}/include"
    "${FORWARDING_HEADERS_DIR}"
    ${CMAKE_BINARY_DIR}
    ${WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR}
    "${CMAKE_SOURCE_DIR}/Source"
    "${TOOLS_DIR}/playstation/"
)

set(MiniBrowser_SOURCES
    ${MINIBROWSER_DIR}/Bookmark.cpp
    ${MINIBROWSER_DIR}/GetLine.cpp
    ${MINIBROWSER_DIR}/MainWindow.cpp
    ${MINIBROWSER_DIR}/Menu.cpp
    ${MINIBROWSER_DIR}/MenuBar.cpp
    ${MINIBROWSER_DIR}/URLBarText.cpp
    ${MINIBROWSER_DIR}/PunchHoleWidget.cpp
    ${MINIBROWSER_DIR}/FocusWidget.cpp
    ${MINIBROWSER_DIR}/WebViewWindow.cpp
    ${MINIBROWSER_DIR}/UserAgent.cpp
    ${MINIBROWSER_DIR}/OpenGLServerProcessProxy.cpp
    ${MINIBROWSER_DIR}/main.cpp
)

set(MiniBrowser_LIBRARIES
    ${C_STD_LIBRARY}
    ${KERNEL_LIBRARY}

    ${TOOLKIT_LIBRARY}

    Cairo::Cairo
    WebKit::WebKit
)

set(PLAYSTATION_MiniBrowser_REQUIRED_SHARED_LIBRARIES
    libToolKitten
    libcairo
    libfontconfig
    libfreetype
    libharfbuzz
    libicu
    libpng16
)

add_definitions(
    -D_UNICODE
    -DTEST_WEBKIT_RESOURCES_DIR=\"${TOOLS_DIR}/TestWebKitAPI/Tests/WebKit\"
)

link_directories(${WEBKIT_LIBRARIES_DIR}/lib)
WEBKIT_EXECUTABLE_DECLARE(MiniBrowser)
WEBKIT_EXECUTABLE(MiniBrowser)

# MiniBrowser won't actually work without some of the processes
add_dependencies(MiniBrowser WebProcess NetworkProcess)

target_link_options(MiniBrowser PRIVATE -Wl,--no-demangle)

if (${CMAKE_GENERATOR} MATCHES "Visual Studio")
    # Set the debugger working directory for Visual Studio
    set_target_properties(MiniBrowser PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

    # Set the startup target to MiniBrowser
    set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MiniBrowser)
endif ()
