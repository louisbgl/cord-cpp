# Each example is one folder with a main.cpp, built to build/examples/<name>/<name>.
# Config paths in the examples are relative to the repo root, so run them from there:
#   ./build/examples/basic/basic

function(cord_add_example name)
    add_executable(example_${name} ${CMAKE_CURRENT_SOURCE_DIR}/${name}/main.cpp)
    target_link_libraries(example_${name} PRIVATE cord)
    set_target_properties(example_${name} PROPERTIES
        OUTPUT_NAME ${name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${name}
    )
endfunction()

cord_add_example(basic)
