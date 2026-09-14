# Runs the CLI with a scripted stdin and checks expected output lines.
# Invoked by CTest; see tests/CMakeLists.txt.
file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(input "${SCRIPT_DIR}/${SCRIPT_NAME}.in")
set(expect "${SCRIPT_DIR}/${SCRIPT_NAME}.expect")
if(NOT EXISTS "${input}" OR NOT EXISTS "${expect}")
    message(FATAL_ERROR "Missing ${input} or ${expect}")
endif()

# A script may contain several sessions separated by a line "### RESTART" to
# test that data survives a restart of the program.
file(READ "${input}" script)
string(REPLACE "### RESTART" ";" sessions "${script}")
set(all_output "")
set(session_no 0)
foreach(session IN LISTS sessions)
    math(EXPR session_no "${session_no} + 1")
    file(WRITE "${WORK_DIR}/session_${session_no}.in" "${session}")
    execute_process(
        COMMAND "${LMS_EXE}" --data "${WORK_DIR}/library.dat" --today 2026-09-14
        INPUT_FILE "${WORK_DIR}/session_${session_no}.in"
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
        RESULT_VARIABLE code
        WORKING_DIRECTORY "${WORK_DIR}")
    if(NOT code EQUAL 0)
        message(FATAL_ERROR "lms exited with code ${code}\n--- stdout ---\n${out}\n--- stderr ---\n${err}")
    endif()
    string(APPEND all_output "${out}")
endforeach()

file(STRINGS "${expect}" expected_lines)
foreach(line IN LISTS expected_lines)
    if(line STREQUAL "")
        continue()
    endif()
    string(FIND "${all_output}" "${line}" pos)
    if(pos EQUAL -1)
        message(FATAL_ERROR "Expected output line not found:\n  ${line}\n--- actual output ---\n${all_output}")
    endif()
endforeach()
message(STATUS "script '${SCRIPT_NAME}' OK (${session_no} session(s))")
