cmake_minimum_required(VERSION 3.16)

project(LibraryManagementSystem
    VERSION 1.0.0
    DESCRIPTION "Console library management system"
    LANGUAGES CXX)

# ---------------------------------------------------------------- options
option(LMS_BUILD_TESTS "Build unit tests" ON)
option(LMS_RUN_TESTS_ON_BUILD "Run the test suite as part of the default build" ON)
option(LMS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd / IDE integration

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(cmake/Warnings.cmake)

if(MSVC)
    # std::getenv & co. are standard; silence MSVC's C4996 "unsafe" deprecation.
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

if(MINGW)
    # Self-contained executables: a MinGW binary that links the C++ runtime
    # dynamically picks up whichever libstdc++-6.dll comes first in PATH
    # (Git for Windows ships an incompatible one) and crashes at startup.
    add_link_options(-static)
endif()

# ------------------------------------------------------------ version.h
configure_file(cmake/version.h.in "${CMAKE_BINARY_DIR}/generated/lms/version.h" @ONLY)

# ---------------------------------------------------------- core library
add_library(lms_core STATIC
    src/core/date.cpp
    src/core/library.cpp
    src/core/money.cpp
    src/core/storage.cpp
    src/core/transaction.cpp)
target_include_directories(lms_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_BINARY_DIR}/generated")
target_compile_features(lms_core PUBLIC cxx_std_17)
lms_set_warnings(lms_core)

# --------------------------------------------------------- console app
add_executable(lms
    src/cli/app.cpp
    src/cli/console.cpp
    src/cli/main.cpp)
target_link_libraries(lms PRIVATE lms_core)
lms_set_warnings(lms)

install(TARGETS lms RUNTIME DESTINATION bin)

# --------------------------------------------------------------- tests
if(LMS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)

    if(LMS_RUN_TESTS_ON_BUILD)
        # "Проверка проекта тестами во время сборки": the default `cmake --build`
        # target depends on this, so a failing test fails the build.
        add_custom_target(run_tests ALL
            COMMAND "${CMAKE_CTEST_COMMAND}" --output-on-failure -C $<CONFIG>
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Running test suite"
            VERBATIM)
        add_dependencies(run_tests lms lms_tests)
    endif()
endif()
