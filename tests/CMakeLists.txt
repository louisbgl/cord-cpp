include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# Runtime tests
set(TEST_SOURCES
    test_parsing.cpp
    test_vectors.cpp
    test_schema.cpp
    test_errors.cpp
    test_comments.cpp
    test_markers.cpp
    test_get_or.cpp
    test_file_parsing.cpp
)

add_executable(cord_tests ${TEST_SOURCES})
target_link_libraries(cord_tests PRIVATE cord Catch2::Catch2WithMain)

if(CORD_COVERAGE)
    target_compile_options(cord_tests PRIVATE --coverage -O0 -g)
    target_link_options(cord_tests PRIVATE --coverage)
endif()

include(CTest)
include(Catch)
catch_discover_tests(cord_tests)

# Compile-fail tests: verify unsupported types are rejected at compile time
foreach(CF_TEST test_type_assertions test_get_or_type_safety)
    try_compile(
        COMPILED_${CF_TEST}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/${CF_TEST}.cpp
        CXX_STANDARD 20
        LINK_LIBRARIES cord
    )
    if(COMPILED_${CF_TEST})
        message(FATAL_ERROR "Compile-fail test '${CF_TEST}' should not compile but did.")
    else()
        message(STATUS "Compile-fail test '${CF_TEST}': OK (correctly rejected)")
    endif()
endforeach()
