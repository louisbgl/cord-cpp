set(EXAMPLES simplest arrays optionals config_markers)

foreach(EXAMPLE ${EXAMPLES})
    set(TARGET example_${EXAMPLE})

    add_executable(${TARGET} ${EXAMPLE}/${EXAMPLE}.cpp)
    target_link_libraries(${TARGET} PRIVATE cord)

    add_custom_target(run_${EXAMPLE}
        COMMAND ${TARGET}
        DEPENDS ${TARGET}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/examples/${EXAMPLE}
        COMMENT "Running example: ${EXAMPLE}"
    )
endforeach()
