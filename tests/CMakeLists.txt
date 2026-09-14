include(FetchContent)

# GoogleTest is downloaded at configure time so the project has no
# system-level test dependency. Pinned to a release tag for reproducibility.
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # MSVC: match runtime of lms_core
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(lms_tests
    test_books.cpp
    test_date.cpp
    test_history.cpp
    test_loans.cpp
    test_money.cpp
    test_search.cpp
    test_storage.cpp
    test_students.cpp)
target_link_libraries(lms_tests PRIVATE lms_core GTest::gtest_main)
# GoogleTest headers are third-party code: treat them as system headers so
# our strict warning set (and -Werror) applies to our tests only.
get_target_property(_gtest_includes gtest INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(lms_tests SYSTEM PRIVATE ${_gtest_includes})
lms_set_warnings(lms_tests)

include(GoogleTest)
gtest_discover_tests(lms_tests DISCOVERY_MODE PRE_TEST)

# ---- CLI smoke tests: run the real binary with scripted input -----------
add_test(NAME cli.version COMMAND lms --version)
set_tests_properties(cli.version PROPERTIES PASS_REGULAR_EXPRESSION "^lms [0-9]+\.[0-9]+\.[0-9]+")

add_test(NAME cli.help COMMAND lms --help)
set_tests_properties(cli.help PROPERTIES PASS_REGULAR_EXPRESSION "Usage: lms")

add_test(NAME cli.bad_option COMMAND lms --bogus)
set_tests_properties(cli.bad_option PROPERTIES WILL_FAIL TRUE)

# Feeds tests/scripts/<name>.in to the program and checks that every line of
# tests/scripts/<name>.expect appears in the output.
function(lms_add_script_test name)
    add_test(NAME cli.script.${name}
        COMMAND "${CMAKE_COMMAND}"
            -DLMS_EXE=$<TARGET_FILE:lms>
            -DSCRIPT_DIR=${CMAKE_CURRENT_SOURCE_DIR}/scripts
            -DSCRIPT_NAME=${name}
            -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/script_${name}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_script.cmake")
    set_tests_properties(cli.script.${name} PROPERTIES ENVIRONMENT "LMS_ADMIN_PASSWORD=secret")
endfunction()

lms_add_script_test(exit_immediately)
lms_add_script_test(wrong_password)
lms_add_script_test(invalid_input)
lms_add_script_test(student_flow)
lms_add_script_test(persistence)
