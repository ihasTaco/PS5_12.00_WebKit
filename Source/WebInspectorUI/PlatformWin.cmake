set(WebInspectorUI_RESOURCES_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/WebKit.resources")
set(WebInspectorUI_LOCALIZED_STRINGS_DIR "${WebInspectorUI_RESOURCES_DIR}/WebInspectorUI")
set(WebInspectorUI_LEGACY_DIR "${WebInspectorUI_LOCALIZED_STRINGS_DIR}/Protocol/Legacy/playstation")

set(JavaScriptCore_INSPECTOR_SCRIPTS_DIR "${JAVASCRIPTCORE_DIR}/inspector/scripts")

set(WebInspectorUI_LEGACY_VERSION_FILES
#TODO: If the backend(playstation) version is updated in the future, add the json file for old version here.
#    ${WEBINSPECTORUI_DIR}/Versions/Inspector-playstation-1.0.json
#    ${WEBINSPECTORUI_DIR}/Versions/Inspector-playstation-2.0.json
)

foreach (_input_file ${WebInspectorUI_LEGACY_VERSION_FILES})
    string(REGEX MATCH "^.*Inspector-playstation-([0-9]+\.[0-9]+)\.json" _matched ${_input_file})
    set(_output_path ${WebInspectorUI_LEGACY_DIR}/${CMAKE_MATCH_1})

    add_custom_command(
        OUTPUT ${_output_path}/InspectorBackendCommands.js
        MAIN_DEPENDENCY ${JavaScriptCore_INSPECTOR_SCRIPTS_DIR}/generate-inspector-protocol-bindings.py
        DEPENDS ${_input_file}
                ${JavaScriptCore_INSPECTOR_SCRIPTS_DIR}/generate-inspector-protocol-bindings.py
        COMMAND ${CMAKE_COMMAND} -E make_directory ${_output_path}
        COMMAND ${PYTHON_EXECUTABLE} ${JavaScriptCore_INSPECTOR_SCRIPTS_DIR}/generate-inspector-protocol-bindings.py --outputDir ${_output_path} --framework WebInspectorUI ${_input_file}
        COMMAND ${PERL_EXECUTABLE} ${JavaScriptCore_INSPECTOR_SCRIPTS_DIR}/codegen/preprocess.pl --input ${_output_path}/InspectorBackendCommands.js.in --defines "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR}" --preprocessor "${CODE_GENERATOR_PREPROCESSOR}" --output ${_output_path}/InspectorBackendCommands.js
        VERBATIM
    )

    list(APPEND InspectorFilesDependencies
        ${_output_path}/InspectorBackendCommands.js
    )
endforeach ()

