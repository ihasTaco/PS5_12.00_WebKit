set(PORT PlayStation)

include(CheckCXXCompilerFlag)
include(CheckSymbolExists)
include(CMakePushCheckState)

string(APPEND CMAKE_C_FLAGS_RELEASE " -g")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -g")
set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(PLAYSTATION_PLATFORM "playstation" CACHE STRING "PlayStation Platform")

include(PlayStationModule)
include(Sign)
include(Shuffle)

WEBKIT_PREPEND_GLOBAL_COMPILER_FLAGS(-sce-stdlib=v2)

add_definitions(-DWTF_PLATFORM_PLAYSTATION=1)
add_definitions(-DBPLATFORM_PLAYSTATION=1)

add_definitions(-DSCE_LIBC_DISABLE_CPP14_HEADER_WARNING= -DSCE_LIBC_DISABLE_CPP17_HEADER_WARNING=)

# bug-224462
WEBKIT_PREPEND_GLOBAL_COMPILER_FLAGS(-Wno-dll-attribute-on-redeclaration)

set(ENABLE_WEBKIT_LEGACY OFF)
set(ENABLE_WEBINSPECTORUI OFF)

# Set ouput directories.
# Libraries (i.e. prx) should be output into
# the same place as CMAKE_RUNTIME_OUTPUT_DIRECTORY

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# FIXME: Remove this ebootparam.ini after "workspace" gets fixed.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
file(TOUCH ${CMAKE_BINARY_DIR}/bin/ebootparam.ini)

# If no CMAKE_INSTALL_PREFIX was given, set a default.
# Put these in the cache, because the CMAKE_INSTALL_PREFIX in the cache
# will be used on following runs, even if it was initialized to default
# on that first run without setting
# CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT on the later runs.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT OR
    NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/dist" CACHE PATH
        "Location to install projects" FORCE)

    # These are set by OptionsCommon before OptionsPlayStation runs
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH
        "Location to install libraries" FORCE)
    set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH
        "Location to install binaries" FORCE)
    set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH
        "Location to install executables executed by the library" FORCE)
endif ()
set(WEBKITPLAYSTATION_HEADER_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "Path to install headers")

# Specify third party library directory
if (NOT WEBKIT_LIBRARIES_DIR)
    if (DEFINED ENV{WEBKIT_LIBRARIES})
        set(WEBKIT_LIBRARIES_DIR "$ENV{WEBKIT_LIBRARIES}" CACHE PATH "Path to PlayStationRequirements")
    else ()
        set(WEBKIT_LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/WebKitLibraries/playstation" CACHE PATH "Path to PlayStationRequirements")
    endif ()
endif ()

if (DEFINED ENV{WEBKIT_IGNORE_PATH})
    set(CMAKE_IGNORE_PATH $ENV{WEBKIT_IGNORE_PATH})
endif ()

list(APPEND CMAKE_PREFIX_PATH ${WEBKIT_LIBRARIES_DIR})

# Find libraries
find_library(C_STD_LIBRARY c)
find_library(KERNEL_LIBRARY kernel)
find_package(ICU 61.2 REQUIRED COMPONENTS data i18n uc)
find_package(Threads REQUIRED)

set(USE_WPE_BACKEND_PLAYSTATION OFF)
set(PlayStationModule_TARGETS ICU::uc)

if (ENABLE_WEBCORE)
    set(WebKitRequirements_COMPONENTS WebKitResources)
    set(WebKitRequirements_OPTIONAL_COMPONENTS
        JPEG
        LibPSL
        LibXml2
        SQLite3
        WebP
        ZLIB
    )

    find_package(WPEBackendPlayStation)
    if (WPEBackendPlayStation_FOUND)
        # WPE::libwpe is compiled into the PlayStation backend
        set(WPE_NAMES SceWPE ${WPE_NAMES})
        find_package(WPE 1.14.0 REQUIRED)

        SET_AND_EXPOSE_TO_BUILD(USE_WPE_BACKEND_PLAYSTATION ON)

        set(ProcessLauncher_LIBRARY WPE::PlayStation)
        list(APPEND PlayStationModule_TARGETS WPE::PlayStation)
    else ()
        set(ProcessLauncher_LIBRARY WebKitRequirements::ProcessLauncher)
        list(APPEND WebKitRequirements_COMPONENTS
            ProcessLauncher
            libwpe
        )
    endif ()

    find_package(WebKitRequirements
        REQUIRED COMPONENTS ${WebKitRequirements_COMPONENTS}
        OPTIONAL_COMPONENTS ${WebKitRequirements_OPTIONAL_COMPONENTS}
    )

    set(Brotli_NAMES SceVshBrotli ${Brotli_NAMES})
    set(Brotli_DEC_NAMES ${Brotli_NAMES} ${Brotli_DEC_NAMES})
    set(Cairo_NAMES SceCairoForWebKit ${Cairo_NAMES})
    set(HarfBuzz_NAMES SceVshHarfbuzz ${HarfBuzz_NAMES})
    set(HarfBuzz_ICU_NAMES ${HarfBuzz_NAMES} ${HarfBuzz_ICU_NAMES})
    # The OpenGL ES implementation is in the same library as the EGL implementation
    set(OpenGLES2_NAMES ${EGL_NAMES})

    find_package(Brotli OPTIONAL_COMPONENTS dec)
    find_package(CURL 7.85.0 REQUIRED)
    find_package(Cairo REQUIRED)
    find_package(EGL REQUIRED)
    find_package(Fontconfig REQUIRED)
    find_package(Freetype REQUIRED)
    find_package(HarfBuzz REQUIRED COMPONENTS ICU)
    find_package(OpenGLES2 REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(PNG REQUIRED)

    list(APPEND PlayStationModule_TARGETS
        Brotli::dec
        CURL::libcurl
        Cairo::Cairo
        Fontconfig::Fontconfig
        Freetype::Freetype
        HarfBuzz::HarfBuzz
        OpenSSL::SSL
        PNG::PNG
        WebKitRequirements::WebKitResources
    )

    if (NOT TARGET JPEG::JPEG)
        find_package(JPEG 1.5.2 REQUIRED)
        list(APPEND PlayStationModule_TARGETS JPEG::JPEG)
    endif ()

    if (NOT TARGET LibPSL::LibPSL)
        set(LibPSL_NAMES SceVshPsl ${LibPSL_NAMES})
        find_package(LibPSL 0.20.2 REQUIRED)
        list(APPEND PlayStationModule_TARGETS LibPSL::LibPSL)
    endif ()

    if (NOT TARGET LibXml2::LibXml2)
        find_package(LibXml2 2.9.7 REQUIRED)
        list(APPEND PlayStationModule_TARGETS LibXml2::LibXml2)
    endif ()

    if (NOT TARGET SQLite::SQLite3)
        find_package(SQLite3 3.23.1 REQUIRED)
        list(APPEND PlayStationModule_TARGETS SQLite::SQLite3)
    endif ()

    if (NOT TARGET WebP::libwebp)
        set(WebP_NAMES SceVshWebP ${WebP_NAMES})
        set(WebP_DEMUX_NAMES ${WebP_NAMES} ${WebP_DEMUX_NAMES})
        find_package(WebP REQUIRED COMPONENTS demux)
        list(APPEND PlayStationModule_TARGETS WebP::libwebp)
    endif ()

    if (NOT TARGET ZLIB::ZLIB)
        find_package(ZLIB 1.2.11 REQUIRED)
        list(APPEND PlayStationModule_TARGETS ZLIB::ZLIB)
    endif ()
endif ()

WEBKIT_OPTION_BEGIN()

# Developer mode options
SET_AND_EXPOSE_TO_BUILD(ENABLE_DEVELOPER_MODE ${DEVELOPER_MODE})
if (ENABLE_DEVELOPER_MODE)
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_API_TESTS PRIVATE ON)
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LAYOUT_TESTS PUBLIC ${USE_WPE_BACKEND_PLAYSTATION})
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MINIBROWSER PUBLIC ${ENABLE_WEBKIT})
endif ()

# PlayStation Specific Options
WEBKIT_OPTION_DEFINE(ENABLE_STATIC_JSC "Control whether to build a non-shared JSC" PUBLIC ON)

# Turn off JIT
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTL_JIT PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_DFG_JIT PRIVATE OFF)

# Don't use IsoMalloc
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_ISO_MALLOC PRIVATE OFF)

# Enabled features
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ACCESSIBILITY PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FULLSCREEN_API PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_TRACKING_PREVENTION PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NETWORK_CACHE_SPECULATIVE_REVALIDATION PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NETWORK_CACHE_STALE_WHILE_REVALIDATE PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_PERIODIC_MEMORY_MONITOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_REMOTE_INSPECTOR PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_RESOURCE_USAGE PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SMOOTH_SCROLLING PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_CRYPTO PRIVATE ON)

if (USE_WPE_BACKEND_PLAYSTATION)
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBDRIVER PRIVATE ON)
endif ()

# Enabled features downstream
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ACCESSIBILITY PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LAYOUT_TESTS PRIVATE ${ENABLE_WEBCORE})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VIDEO PRIVATE ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBDRIVER PUBLIC ${ENABLE_WEBCORE})

# Experimental features
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_APPLICATION_MANIFEST PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_PAINTING_API PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FILTERS_LEVEL_2 PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_LAYER_BASED_SVG_ENGINE PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_SERVICE_WORKER PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})

if (USE_WPE_BACKEND_PLAYSTATION)
    WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GAMEPAD PRIVATE ${ENABLE_EXPERIMENTAL_FEATURES})
endif ()

# Features to investigate
#
# Features that require additional implementation pieces
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_AVIF PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(USE_JPEGXL PRIVATE OFF)

# Features that are temporarily turned off because an implementation is not
# present at this time
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_ASYNC_SCROLLING PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GPU_PROCESS PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEB_AUDIO PRIVATE OFF)

# Reenable after updating fontconfig
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_VARIATION_FONTS PRIVATE OFF)

# Enable in the future
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CONTEXT_MENUS PRIVATE OFF)

# No support planned
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FTPDIR PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_GEOLOCATION PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_IMAGE_DIFF PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_MATHML PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NETSCAPE_PLUGIN_API PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_NOTIFICATIONS PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_USER_MESSAGE_HANDLERS PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_WEBGL PRIVATE OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_XSLT PRIVATE OFF)

# Enabled features
WEBKIT_OPTION_DEFINE(ENABLE_CURSOR_NAVIGATION PRIVATE "Toggle cursor navigation support" ON)
WEBKIT_OPTION_DEFINE(ENABLE_PUNCH_HOLE PRIVATE "Toggle Punch Hole support" ON)

# Disable CSS_COMPOSITING because it doesn't work well with PUNCH_HOLE feature
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_CSS_COMPOSITING PRIVATE OFF)

if (NOT ENABLE_WEBCORE)
    WEBKIT_OPTION_DEFINE(ENABLE_LIBPAS PRIVATE "Toggle libpas support" OFF)
else ()
    WEBKIT_OPTION_DEFINE(ENABLE_LIBPAS PRIVATE "Toggle libpas support" OFF)
endif ()

# Enable Memory Analyzer support
WEBKIT_OPTION_DEFINE(ENABLE_MAT "Enable Memory Analyzer support" PUBLIC OFF)

WEBKIT_OPTION_END()

# Do not use a separate directory based on configuration when building
# with the Visual Studio generator
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Default to hidden visibility
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_C_STANDARD_LIBRARIES
    "${CMAKE_C_STANDARD_LIBRARIES} ${C_STD_LIBRARY}"
)
set(CMAKE_CXX_STANDARD_LIBRARIES
    "${CMAKE_CXX_STANDARD_LIBRARIES} ${C_STD_LIBRARY}"
)

SET_AND_EXPOSE_TO_BUILD(HAVE_PTHREAD_SETNAME_NP ON)
SET_AND_EXPOSE_TO_BUILD(HAVE_MAP_ALIGNED OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "PlayStation4")
    WEBKIT_PREPEND_GLOBAL_COMPILER_FLAGS(-Wno-builtin-macro-redefined)
    add_compile_options(-U__cpp_char8_t)
    SET_AND_EXPOSE_TO_BUILD(HAVE_MISSING_STD_FILESYSTEM_PATH_CONSTRUCTOR ON)
else ()
    SET_AND_EXPOSE_TO_BUILD(HAVE_MISSING_STD_FILESYSTEM_PATH_CONSTRUCTOR OFF)
endif ()

# Platform options
SET_AND_EXPOSE_TO_BUILD(USE_INSPECTOR_SOCKET_SERVER ${ENABLE_REMOTE_INSPECTOR})
SET_AND_EXPOSE_TO_BUILD(USE_UNIX_DOMAIN_SOCKETS ON)

if (ENABLE_WEBCORE)
    SET_AND_EXPOSE_TO_BUILD(USE_CAIRO ON)
    SET_AND_EXPOSE_TO_BUILD(USE_CURL ON)
    SET_AND_EXPOSE_TO_BUILD(USE_FREETYPE ON)
    SET_AND_EXPOSE_TO_BUILD(USE_HARFBUZZ ON)
    SET_AND_EXPOSE_TO_BUILD(USE_LIBWPE ON)
    SET_AND_EXPOSE_TO_BUILD(USE_OPENSSL ON)
    SET_AND_EXPOSE_TO_BUILD(USE_WEBP ON)

    # See if OpenSSL implementation is BoringSSL
    cmake_push_check_state()
    set(CMAKE_REQUIRED_INCLUDES "${OPENSSL_INCLUDE_DIR}")
    set(CMAKE_REQUIRED_LIBRARIES "${OPENSSL_LIBRARIES}")
    WEBKIT_CHECK_HAVE_SYMBOL(USE_BORINGSSL OPENSSL_IS_BORINGSSL openssl/ssl.h)
    cmake_pop_check_state()

    # See if FreeType implementation supports WOFF2 fonts
    include(WOFF2Checks)
    if (FREETYPE_WOFF2_SUPPORT_IS_AVAILABLE)
        SET_AND_EXPOSE_TO_BUILD(HAVE_WOFF_SUPPORT ON)
    endif ()

    # Rendering options
    SET_AND_EXPOSE_TO_BUILD(USE_EGL ON)
    SET_AND_EXPOSE_TO_BUILD(USE_TEXTURE_MAPPER ON)
    SET_AND_EXPOSE_TO_BUILD(USE_TEXTURE_MAPPER_GL ON)
    SET_AND_EXPOSE_TO_BUILD(USE_WPE_RENDERER ${USE_WPE_BACKEND_PLAYSTATION})

    if (ENABLE_GPU_PROCESS)
        SET_AND_EXPOSE_TO_BUILD(USE_GRAPHICS_LAYER_TEXTURE_MAPPER ON)
        SET_AND_EXPOSE_TO_BUILD(USE_GRAPHICS_LAYER_WC ON)
    else ()
        SET_AND_EXPOSE_TO_BUILD(ENABLE_SCROLLING_THREAD ${ENABLE_ASYNC_SCROLLING})

        SET_AND_EXPOSE_TO_BUILD(USE_COORDINATED_GRAPHICS ON)
        SET_AND_EXPOSE_TO_BUILD(USE_NICOSIA ON)
    endif ()
else ()
    SET_AND_EXPOSE_TO_BUILD(USE_OVERRIDABLE_TARGET_DISPATCHER ON)
endif ()

# LLint
SET_AND_EXPOSE_TO_BUILD(ENABLE_LLINT_EMBEDDED_OPCODE_ID OFF)

# WebDriver options
SET_AND_EXPOSE_TO_BUILD(ENABLE_WEBDRIVER_KEYBOARD_INTERACTIONS ON)
SET_AND_EXPOSE_TO_BUILD(ENABLE_WEBDRIVER_MOUSE_INTERACTIONS ON)

if (ENABLE_MINIBROWSER)
    find_library(TOOLKIT_LIBRARY ToolKitten)
    if (NOT TOOLKIT_LIBRARY)
        message(FATAL_ERROR "ToolKit library required to run MiniBrowser")
    endif ()
endif ()

# Disable SCROLLING_THREAD for now
SET_AND_EXPOSE_TO_BUILD(ENABLE_SCROLLING_THREAD PRIVATE OFF)

# Stop using <sys/timeb.h>
SET_AND_EXPOSE_TO_BUILD(HAVE_SYS_TIMEB_H OFF)

# Override headers directories
set(ANGLE_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/ANGLE/Headers)
set(WTF_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WTF/Headers)
set(JavaScriptCore_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/JavaScriptCore/Headers)
set(JavaScriptCore_PRIVATE_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/JavaScriptCore/PrivateHeaders)
set(PAL_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/PAL/Headers)
set(WebCore_PRIVATE_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WebCore/PrivateHeaders)
set(WebKitLegacy_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WebKitLegacy/Headers)
set(WebKit_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WebKit/Headers)
set(WebKit_PRIVATE_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WebKit/PrivateHeaders)

# Override scripts directories
set(WTF_SCRIPTS_DIR ${CMAKE_BINARY_DIR}/WTF/Scripts)
set(JavaScriptCore_SCRIPTS_DIR ${CMAKE_BINARY_DIR}/JavaScriptCore/Scripts)

# Create a shared JavaScriptCore with WTF and bmalloc exposed through it.
#
# Use OBJECT libraries for bmalloc and WTF. This is the modern CMake way to emulate
# the behavior of --whole-archive. If this is not done then all the exports will
# not be exposed.
set(bmalloc_LIBRARY_TYPE OBJECT)
set(WTF_LIBRARY_TYPE OBJECT)

if (ENABLE_STATIC_JSC)
    set(JavaScriptCore_LIBRARY_TYPE OBJECT)
else ()
    set(JavaScriptCore_LIBRARY_TYPE SHARED)
endif ()

# Create a shared WebKit
#
# Use OBJECT libraries for PAL and WebCore. The size of a libWebCore.a is too much
# for ranlib.
set(PAL_LIBRARY_TYPE OBJECT)
set(WebCore_LIBRARY_TYPE OBJECT)
set(WebKit_LIBRARY_TYPE SHARED)

# Enable stack protector
add_definitions(-fstack-protector-strong)

# Disable "undef" macro warning
add_definitions(-Wno-undef)

find_library(MEMORY_EXTRA_LIB MemoryExtra)
find_path(MEMORY_EXTRA_INCLUDE_DIR NAMES memory-extra)

if (ENABLE_MAT)
    find_package(libmat CONFIG REQUIRED)
    link_libraries(libmat::mat)
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    foreach (PREFIX_HEADER IN LISTS PLAYSTATION_PREFIX_HEADERS)
        add_definitions(-include ${PREFIX_HEADER})
    endforeach ()
else ()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} is not supported.")
endif ()

# Enable multi process builds for Visual Studio
if (${CMAKE_GENERATOR} MATCHES "Ninja")
    # We use CMAKE_INTDIR for finding the prxs
    # MSVC puts the executable/prxs in a subdir by build type
    # Ninja does not, and doesn't set CMAKE_INTDIR, so put in an empty
    add_definitions(-DCMAKE_INTDIR="")
else ()
    add_definitions(/MP)
endif ()

find_package(libdl)
if (TARGET libdl::dl)
    add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:$<TARGET_PROPERTY:libdl::dl,IMPORTED_LOCATION_RELEASE>>")
    add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:$<TARGET_PROPERTY:libdl::dl,INTERFACE_LINK_OPTIONS>>")
else ()
    find_library(DL_LIBRARY NAMES dl PATHS ${WEBKIT_LIBRARIES_DIR}/lib)
    if (DL_LIBRARY)
        add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${DL_LIBRARY}>")
        add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--wrap=dlopen>")
        add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--wrap=dlclose>")
        add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--wrap=dlerror>")
        add_link_options("$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--wrap=dlsym>")
    endif ()
endif ()

function(add_executable target)
    _add_executable(${ARGV})
    target_link_options(${target} PRIVATE -Wl,--wrap=mmap)
endfunction()

function(add_library target type)
    _add_library(${ARGV})
    if ("${type}" STREQUAL "SHARED")
        target_link_options(${target} PRIVATE -Wl,--wrap=mmap)
        sign(${target})
    endif ()
endfunction()

find_file(SHUFFLING_SEED shuffling_seed.bin PATHS ${WEBKIT_LIBRARIES_DIR}/tools/shuffling)

macro(WEBKIT_FRAMEWORK_FOR_RELEASE _target)
    if (NOT SHUFFLING_SEED)
        message(FATAL_ERROR "Shuffling seed cannot be found.")
    endif ()

    if (LTO_MODE)
        target_link_options(${_target} PRIVATE -Wl,--lto-O0)
    endif ()

    set(${_target}_SHUFFLED_SYMLIST ${CMAKE_CURRENT_BINARY_DIR}/${_target}_symlist.txt)
    shuffle(${_target} ${SHUFFLING_SEED} ${${_target}_SHUFFLED_SYMLIST})

    # Adding a release target prefixed with SceNK, taking over cmake parameters from the original framework target.
    set(SceNK${_target}_LIBRARY_TYPE SHARED)
    WEBKIT_FRAMEWORK_DECLARE(SceNK${_target})
    _WEBKIT_TARGET(${_target} SceNK${_target})

    # Wrap symbols
    target_link_options(SceNK${_target} PRIVATE --wrap=access --wrap=lstat --wrap=getcwd --wrap=link --wrap=symlink --wrap=statvfs)
    target_link_options(${_target} PRIVATE --wrap=access --wrap=lstat --wrap=getcwd --wrap=link --wrap=symlink --wrap=statvfs)

    # Strip unused data and duplicates
    target_link_options(${_target} PRIVATE --gc-sections --icf=all --ignore-data-address-equality)
    target_link_options(SceNK${_target} PRIVATE --gc-sections --icf=all --ignore-data-address-equality)

    # Shuffle symbols
    if (PLAYSTATION_LINKER_ORDER_ARGUMENT)
        target_link_options(SceNK${_target} PRIVATE ${PLAYSTATION_LINKER_ORDER_ARGUMENT}=${${_target}_SHUFFLED_SYMLIST})
    else ()
        target_link_options(SceNK${_target} PRIVATE --order=${${_target}_SHUFFLED_SYMLIST})
    endif ()

    if (LTO_MODE)
        target_link_options(SceNK${_target} PRIVATE -Wl,--lto-O2)
        target_link_options(SceNK${_target} PRIVATE -Wl,-mllvm -Wl,-import-instr-limit=5)
    endif ()

    add_dependencies(SceNK${_target} Shuffle${_target}Symbols ${_target})

    if (NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
        install(TARGETS SceNK${_target}
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            RESOURCE DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif ()
endmacro()

macro(JSC_FRAMEWORK_FOR_RELEASE _target)
    set(JSC_NAME SceJsc)

    set(EXPORT_EMD ${WEBKIT_LIBRARIES_DIR}/playstation/tools/SceJsc/export.emd)
    set(EXPORT_PEMD ${CMAKE_CURRENT_BINARY_DIR}/export.pemd)
    message("${EXPORT_PEMD}")
    add_custom_command(
        OUTPUT ${EXPORT_PEMD}
        COMMAND ${CMAKE_CXX_COMPILER} -x c++ -o ${EXPORT_PEMD} -E ${EXPORT_EMD} ${EXPORT_OPTIONS}
        DEPENDS ${EXPORT_EMD}
    )

    set(${JSC_NAME}_SOURCES ${EXPORT_PEMD})
    set(${JSC_NAME}_FRAMEWORKS ${${_target}_FRAMEWORKS})

    set(${JSC_NAME}_LIBRARY_TYPE SHARED)
    WEBKIT_FRAMEWORK_DECLARE(${JSC_NAME})

    # TODO:
    # This isn't an executable, but we don't want it to become part of the framework stack, either.
    # This also doesn't quite do what we want, because we're asking _WEBKIT_TARGET to base itself
    # on JavaScriptCore, but the private libraries may be different if JavaScriptCore is an
    # object library.
    _WEBKIT_TARGET_LINK_FRAMEWORK(${JSC_NAME})
    _WEBKIT_TARGET(${_target} ${JSC_NAME})

    # TODO: see above, because _WEBKIT_TARGET will miss these, add them here if JavaScriptCore
    # is an object library.
    if (${${_target}_LIBRARY_TYPE} STREQUAL "OBJECT")
        target_link_libraries(${JSC_NAME} PRIVATE ${${JSC_NAME}_PRIVATE_LIBRARIES})
    endif ()

    add_dependencies(${JSC_NAME} ${_target})

    # Wrap symbols
    target_link_options(${JSC_NAME} PRIVATE --wrap=access --wrap=lstat --wrap=getcwd --wrap=link --wrap=symlink --wrap=getenv --wrap=clock_gettime)
    if (LTO_MODE)
        target_link_options(${JSC_NAME} PRIVATE -Wl,--lto-O2)
        target_link_options(${JSC_NAME} PRIVATE -Wl,-mllvm -Wl,-import-instr-limit=5)
    endif ()

    if (NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
        install(TARGETS ${JSC_NAME}
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            RESOURCE DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif ()
endmacro()

macro(WEBKIT_FRAMEWORK _target)
    _WEBKIT_FRAMEWORK(${_target})

    if (${_target} STREQUAL "WebKit" AND ${${_target}_LIBRARY_TYPE} STREQUAL "SHARED")
        WEBKIT_FRAMEWORK_FOR_RELEASE(${_target})
        WEBKIT_ADD_TARGET_CXX_FLAGS(SceNK${_target} -Wno-unused-parameter)
    endif ()

    if (${_target} STREQUAL "JavaScriptCore" AND NOT ENABLE_WEBCORE)
        JSC_FRAMEWORK_FOR_RELEASE(${_target})
    endif ()
endmacro()

add_custom_target(playstation_tools_copy
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${WEBKIT_LIBRARIES_DIR}/tools/sce_sys/
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sce_sys/
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${WEBKIT_LIBRARIES_DIR}/tools/scripts/
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scripts/
    COMMAND ${CMAKE_COMMAND} -E touch
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ebootparam.ini
)
set_target_properties(playstation_tools_copy PROPERTIES FOLDER "PlayStation")

macro(WEBKIT_EXECUTABLE _target)
    _WEBKIT_EXECUTABLE(${_target})
    if (LTO_MODE)
        target_link_options(${_target} PRIVATE -Wl,--lto=thin)
    endif ()
    if (NOT PLAYSTATION_${_target}_AVOID_LIBC_SETUP)
        playstation_setup_libc(${_target})
    endif ()
    playstation_setup_fp(${_target})
    if (NOT ${_target} MATCHES "^LLInt")
        sign(${_target})
    endif ()
    if (PLAYSTATION_${_target}_WRAP)
        foreach (WRAP ${PLAYSTATION_${_target}_WRAP})
            target_link_options(${_target} PRIVATE -Wl,--wrap=${WRAP})
        endforeach ()
    endif ()
    add_dependencies(${_target} playstation_tools_copy)
    if (PLAYSTATION_${_target}_REQUIRED_SHARED_LIBRARIES)
        foreach (SLIB ${PLAYSTATION_${_target}_REQUIRED_SHARED_LIBRARIES})
            find_file(${_target}_${SLIB}_SHARED_LIBRARY
                NAMES ${SLIB}.sprx
                PATHS ${WEBKIT_LIBRARIES_DIR}/bin
                NO_DEFAULT_PATH
            )
            if (${_target}_${SLIB}_SHARED_LIBRARY)
                add_custom_command(TARGET ${_target}
                    POST_BUILD
                    COMMAND ${CMAKE_COMMAND}
                            -E copy
                            ${${_target}_${SLIB}_SHARED_LIBRARY}
                            $<TARGET_FILE_DIR:${_target}>
                )
            endif ()
        endforeach ()
    endif ()
endmacro()

# Find PS4 specific libraries
find_library(C_STD_LIBRARY c)
find_library(KERNEL_LIBRARY kernel_stub_weak)
find_library(NET_LIBRARY SceNet_stub_weak)

find_library(WEBKIT_REQUIREMENTS_LIBRARY SceNKWebKitRequirements)
find_library(TOOLKIT_LIBRARY ToolKitten)

set(NETWORK_LIBRARIES ${NET_LIBRARY})

# Workaround: We don't have gmtime_r
add_definitions(-Dgmtime_r=gmtime_s)

# MEDIA

# Check MEDIAPLAYER_LIBRARY version
find_library(MEDIAPLAYER_LIBRARY mediaplayer)
if (EXISTS "${MEDIAPLAYER_LIBRARY}" AND EXISTS "${WEBKIT_LIBRARIES_DIR}/include/mediaplayer/PSMediaPlayerVersion.h")
    file(STRINGS "${WEBKIT_LIBRARIES_DIR}/include/mediaplayer/PSMediaPlayerVersion.h" _contents REGEX "^#define MEDIAPLAYER_API_VERSION+")
    if (_contents)
        string(REGEX REPLACE [[^#define MEDIAPLAYER_API_VERSION[ \t]+([0-9]+)]] [[\1]] _OUT_major "${_contents}")
        if (${_OUT_major} MATCHES "[2-]+")
            set(ENABLE_MEDIA_SOURCE    ON CACHE BOOL "Toggle Media Source support" FORCE)
            set(ENABLE_ENCRYPTED_MEDIA ON CACHE BOOL "Toggle EME support" FORCE)
        endif ()
    endif ()
endif ()

if (ENABLE_MEDIA_SOURCE)
    set(FEATURE_DEFINES_WITH_SPACE_SEPARATOR "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR} ENABLE_MEDIA_SOURCE")
endif ()

if (ENABLE_ENCRYPTED_MEDIA)
    set(FEATURE_DEFINES_WITH_SPACE_SEPARATOR "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR} ENABLE_ENCRYPTED_MEDIA")
endif ()

macro(PLAYSTATION_MODULES)
    set(oneValueArgs DESTINATION)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(opt "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach (_target IN LISTS opt_TARGETS)
        string(REGEX MATCH "^(.+)::(.+)$" _is_stub "${_target}")
        set(_target_name ${CMAKE_MATCH_1})
        playstation_module(${_target_name} TARGET ${_target} FOLDER "PlayStation")

        if (${_target_name}_IS_SYSMODULE)
            EXPOSE_VARIABLE_TO_BUILD(${_target_name}_IS_SYSMODULE)
        endif ()
        if (${_target_name}_LOAD_AT)
            EXPOSE_STRING_VARIABLE_TO_BUILD(${_target_name}_LOAD_AT)
        endif ()
    endforeach ()
endmacro()

macro(PLAYSTATION_COPY_MODULES _target_name)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(opt "" "" "${multiValueArgs}" ${ARGN})

    foreach (_target IN LISTS opt_TARGETS)
        if (TARGET ${_target}_CopyModule)
            list(APPEND ${_target_name}_INTERFACE_DEPENDENCIES ${_target}_CopyModule)
        endif ()
    endforeach ()
endmacro()

PLAYSTATION_MODULES(TARGETS ${PlayStationModule_TARGETS})

# These should be made into proper CMake targets
if (EGL_LIBRARIES)
    playstation_module(EGL TARGET ${EGL_LIBRARIES} FOLDER "PlayStation")
    if (EGL_LOAD_AT)
        EXPOSE_STRING_VARIABLE_TO_BUILD(EGL_LOAD_AT)
    endif ()
endif ()

if (TOOLKIT_LIBRARY)
    playstation_module(ToolKitten TARGET ${TOOLKIT_LIBRARY} FOLDER "PlayStation")
    if (ToolKitten_LOAD_AT)
        EXPOSE_STRING_VARIABLE_TO_BUILD(ToolKitten_LOAD_AT)
    endif ()
endif ()

CMAKE_PUSH_CHECK_STATE()
set(CMAKE_REQUIRED_INCLUDES ${WEBKIT_LIBRARIES_DIR}/include)
check_include_file_cxx(slimglipc.h HAVE_OPENGL_SERVER_SUPPORT_VALUE)
SET_AND_EXPOSE_TO_BUILD(HAVE_OPENGL_SERVER_SUPPORT HAVE_OPENGL_SERVER_SUPPORT_VALUE)
CMAKE_POP_CHECK_STATE()

if (NOT ENABLE_LIBPAS)
    add_definitions(-DBENABLE_LIBPAS=0)
endif ()

EXPOSE_VARIABLE_TO_BUILD(ENABLE_WEBCORE)

check_symbol_exists(memmem string.h HAVE_MEMMEM)
if (HAVE_MEMMEM)
    add_definitions(-DHAVE_MEMMEM=1)
endif ()

if (ENABLE_SANITIZERS)
    foreach (SANITIZER ${ENABLE_SANITIZERS})
        if (${SANITIZER} MATCHES "cfi-debug")
            find_library(CFIHANDLER_LIBRARY NAMES CFIHandler PATHS ${WEBKIT_LIBRARIES_DIR}/lib)
            if (NOT CFIHANDLER_LIBRARY)
                message(FATAL_ERROR "Cannot find CFIHandler library. Cannot build cfi-debug")
            endif()
            add_link_options("$<$<OR:$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>,$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>>:${CFIHANDLER_LIBRARY}>")
        endif()
    endforeach()
endif()
