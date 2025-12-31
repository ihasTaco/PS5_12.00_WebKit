set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

WEBKIT_APPEND_GLOBAL_COMPILER_FLAGS(-Wno-typedef-redefinition)

list(APPEND bmalloc_SOURCES
    bmalloc/playstation/vss-wrapper.cpp
)

list(APPEND bmalloc_PRIVATE_INCLUDE_DIRECTORIES
    ${MEMORY_EXTRA_INCLUDE_DIR}
    "${BMALLOC_DIR}/bmalloc/playstation"
)

if (ENABLE_MAT)
    add_compile_definitions(BENABLE_MAT=1)
    list(APPEND bmalloc_PRIVATE_LIBRARIES libmat::mat)
endif ()

install(TARGETS bmalloc OBJECTS DESTINATION "${LIB_INSTALL_DIR}")
