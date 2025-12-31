include(inspector/remote/Socket.cmake)

list(APPEND JavaScriptCore_SOURCES
    ${CMAKE_SOURCE_DIR}/Source/WebInspectorUI/UserInterface/Protocol/Legacy/playstation/InspectorBackendCommands.cpp
)

list(APPEND JavaScriptCore_PRIVATE_INCLUDE_DIRECTORIES ${MEMORY_EXTRA_INCLUDE_DIR})

target_link_libraries(LLIntSettingsExtractor PRIVATE ${MEMORY_EXTRA_LIB})
target_link_libraries(LLIntOffsetsExtractor PRIVATE ${MEMORY_EXTRA_LIB})

if (ENABLE_MAT)
    list(APPEND JavaScriptCore_PRIVATE_LIBRARIES libmat::mat)
endif ()

if (NOT ENABLE_WEBCORE)
    list(APPEND JavaScriptCore_SOURCES
        API/playstation/JSContextRefPrivatePlayStation.cpp
    )

    list(APPEND JavaScriptCore_PUBLIC_FRAMEWORK_HEADERS
        API/JSBasePrivate.h
        API/JSContextRefPrivate.h
        API/JSHeapFinalizerPrivate.h
        API/JSRemoteInspectorServer.h

        API/playstation/JSContextRefPrivatePlayStation.h
    )

    list(REMOVE_ITEM JavaScriptCore_PRIVATE_FRAMEWORK_HEADERS
        API/JSBasePrivate.h
    )
endif ()

# This overrides the default x64 value of 1GB for the memory pool size
list(APPEND JavaScriptCore_PRIVATE_DEFINITIONS
    FIXED_EXECUTABLE_MEMORY_POOL_SIZE_IN_MB=64
)

if (DEVELOPER_MODE)
    add_subdirectory(testmem)
endif ()

install(FILES
    ${JavaScriptCore_PUBLIC_FRAMEWORK_HEADERS}
    API/JSBasePrivate.h
    API/JSContextRefPrivate.h
    API/JSHeapFinalizerPrivate.h
    API/JSRemoteInspector.h
    DESTINATION "${WEBKITPLAYSTATION_HEADER_INSTALL_DIR}/JavaScriptCore"
)

install(TARGETS JavaScriptCore DESTINATION "${LIB_INSTALL_DIR}")
