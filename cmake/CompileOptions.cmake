
add_library(entity_compile_options INTERFACE)

if(MSVC)
    target_compile_options(
        entity_compile_options
        INTERFACE
            /EHsc
            /W4
    )
else()
    target_compile_options(
        entity_compile_options
        INTERFACE
            $<$<CXX_COMPILER_ID:GNU,Clang>:
                -Wall
                -Wextra
                -Wpedantic
            >
    )
endif()
