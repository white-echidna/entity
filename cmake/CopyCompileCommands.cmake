
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(copy_compile_commands
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
        COMMENT "Updating compile_commands.json"
    )

    if(TARGET entity_compile_options)
        add_dependencies(entity_compile_options copy_compile_commands)
    endif()
endif()
