file(GLOB_RECURSE DERIVED_SOURCES_DIRECTORIES LIST_DIRECTORIES true RELATIVE ${CMAKE_BINARY_DIR} "DerivedSources$")
list(FILTER DERIVED_SOURCES_DIRECTORIES INCLUDE REGEX "DerivedSources$")
# Now the derived sources .o files will match the above, so remove those by culling the
# items (as those directories have __/__ in their names)
list(FILTER DERIVED_SOURCES_DIRECTORIES EXCLUDE REGEX "__/__")

execute_process(COMMAND
    ${CMAKE_COMMAND} -E tar
       "cfv"
       "${CMAKE_INSTALL_PREFIX}/derived_sources.zip"
       --format=zip
       ${DERIVED_SOURCES_DIRECTORIES})
