if (ENABLE_WEBKIT)
    find_file(FILELIST neko_filelist.txt PATH ${WEBKIT_LIBRARIES_DIR}/tools/conf/)
else ()
    find_file(FILELIST jsc_filelist.txt PATH ${WEBKIT_LIBRARIES_DIR}/tools/SceJsc/)
endif ()

if (NOT FILELIST)
    message(FATAL_ERROR "Filelist for packaging not found.")
endif ()

# Expose the filelist path to the install scripts
install(CODE "set(FILELIST \"${FILELIST}\")")
install(CODE "set(ENABLE_WEBKIT ${ENABLE_WEBKIT})")

install(SCRIPT "derived_sources.cmake")
install(SCRIPT "dist.cmake")
