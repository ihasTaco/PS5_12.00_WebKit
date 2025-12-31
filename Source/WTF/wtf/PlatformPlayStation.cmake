list(APPEND WTF_PUBLIC_HEADERS
    unix/UnixFileDescriptor.h
)

list(APPEND WTF_SOURCES
    generic/MainThreadGeneric.cpp
    generic/MemoryFootprintGeneric.cpp
    generic/RunLoopGeneric.cpp
    generic/WorkQueueGeneric.cpp

    playstation/FileSystemPlayStation.cpp
    playstation/LanguagePlayStation.cpp
    playstation/OSAllocatorPlayStation.cpp
    playstation/UniStdExtrasPlayStation.cpp

    posix/CPUTimePOSIX.cpp
    posix/FileSystemPOSIX.cpp
    posix/ThreadingPOSIX.cpp

    text/unix/TextBreakIteratorInternalICUUnix.cpp

    unix/LoggingUnix.cpp
    unix/MemoryPressureHandlerUnix.cpp
)

list(APPEND WTF_PRIVATE_INCLUDE_DIRECTORIES
    ${MEMORY_EXTRA_INCLUDE_DIR}
)

list(APPEND WTF_LIBRARIES
    ${KERNEL_LIBRARY}
    Threads::Threads
)

if (ENABLE_MAT)
    list(APPEND WTF_PRIVATE_LIBRARIES libmat::mat)
endif ()

PLAYSTATION_COPY_MODULES(WTF
    TARGETS
        ICU
)

add_definitions(-DDEFAULT_THREAD_STACK_SIZE_IN_KB=64)

# bmalloc is compiled as an OBJECT library so it is statically linked
list(APPEND WTF_PRIVATE_DEFINITIONS STATICALLY_LINKED_WITH_bmalloc)

install(TARGETS WTF DESTINATION "${LIB_INSTALL_DIR}")
