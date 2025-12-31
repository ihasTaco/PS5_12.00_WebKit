set(WebDriver_OUTPUT_NAME WebDriver)

list(APPEND WebDriver_SOURCES
    playstation/WebDriverServicePlayStation.cpp

    socket/CapabilitiesSocket.cpp
    socket/HTTPParser.cpp
    socket/HTTPServerSocket.cpp
    socket/SessionHostSocket.cpp
)

list(APPEND WebDriver_PRIVATE_INCLUDE_DIRECTORIES
    ${JavaScriptCore_DERIVED_SOURCES_DIR}
    "${WEBDRIVER_DIR}/socket"
)

list(APPEND WebDriver_FRAMEWORKS
    JavaScriptCore
)

set(WebDriver_LIBRARIES ${WEBKIT_REQUIREMENTS_LIBRARY})

set(PLAYSTATION_WebDriver_REQUIRED_SHARED_LIBRARIES
    libSceNKWebKitRequirements
)

WEBKIT_WRAP_EXECUTABLE(WebDriver
    SOURCES ${WEBDRIVER_DIR}/playstation/main.cpp
    TARGET_NAME SceNKWebDriver
)
sign(SceNKWebDriver)

install(TARGETS SceNKWebDriver
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    RESOURCE DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if (PLAYSTATION_BUILD_INFO)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWEBDRIVER_VERSION=\"${PLAYSTATION_BUILD_INFO}\"")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWEBDRIVER_VERSION=\"${PLAYSTATION_BUILD_INFO}\"")
endif ()
