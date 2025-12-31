# Compress artifacts
if (ENABLE_WEBKIT)
    set(PACKAGE_NAME "neko_dist.zip")
else ()
    set(PACKAGE_NAME "jsc.zip")
endif ()

execute_process(COMMAND
    ${CMAKE_COMMAND} -E tar
       "cfv"
       ${PACKAGE_NAME}
       --format=zip
       --files-from=${FILELIST}
       "derived_sources.zip"
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})
