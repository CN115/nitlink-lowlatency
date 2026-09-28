# Only the standalone software-rendered host is copied; no viewer or hardware is opened.
if(NOT DEFINED TEST_EXE OR NOT DEFINED TEST_DIR)
    message(FATAL_ERROR "Missing isolated test paths")
endif()
file(MAKE_DIRECTORY "${TEST_DIR}")
get_filename_component(TEST_NAME "${TEST_EXE}" NAME)
file(COPY "${TEST_EXE}" DESTINATION "${TEST_DIR}")
file(REMOVE "${TEST_DIR}/nitlink-menu.html")
foreach(mode missing empty)
    if(mode STREQUAL "empty")
        file(WRITE "${TEST_DIR}/nitlink-menu.html" "")
    endif()
    execute_process(COMMAND "${TEST_DIR}/${TEST_NAME}" --expect-missing-assets
        WORKING_DIRECTORY "${TEST_DIR}" RESULT_VARIABLE result TIMEOUT 25)
    if(NOT result STREQUAL "0")
        message(FATAL_ERROR "${mode} assets test failed: ${result}")
    endif()
endforeach()

get_filename_component(TEST_SOURCE "${TEST_EXE}" DIRECTORY)
file(COPY "${TEST_SOURCE}/nitlink-menu.html" "${TEST_SOURCE}/assets" "${TEST_SOURCE}/locales"
    DESTINATION "${TEST_DIR}")
file(WRITE "${TEST_DIR}/assets/menu/menu.js" "// Fixture intentionally never posts ready.\n")
execute_process(COMMAND "${TEST_DIR}/${TEST_NAME}" --expect-ready-timeout
    WORKING_DIRECTORY "${TEST_DIR}" RESULT_VARIABLE result TIMEOUT 25)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Missing ready-message test failed: ${result}")
endif()
