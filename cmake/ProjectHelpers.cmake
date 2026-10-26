include_guard(GLOBAL)

include(GenerateExportHeader)
include(GNUInstallDirs)

# NOTE: Order matters.
# `CopyCompileCommands` relies on `entity_compile_options` target.
include(CompileOptions)
include(CopyCompileCommands)

function(melt_sources target_name)
    set(multi_value_args PRIVATE PUBLIC INTERFACE)
    cmake_parse_arguments(ARGS "" "" "${multi_value_args}" ${ARGN})

    target_sources(
        ${target_name}
        PRIVATE
            ${ARGS_PRIVATE}
        PUBLIC
            FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
            FILES
                ${ARGS_PUBLIC}
        INTERFACE
            FILE_SET interface_headers
            TYPE HEADERS
            BASE_DIRS
                "${CMAKE_CURRENT_SOURCE_DIR}/include"
            FILES
                ${ARGS_INTERFACE}
    )
endfunction()

function(melt_export_header target_name)
    set(one_value_args BASE_NAME FILE_NAME)
    cmake_parse_arguments(ARGS "" "${one_value_args}" "" ${ARGN})

    set(_base_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
    set(_file_path "${_base_dir}/${ARGS_FILE_NAME}")

    generate_export_header(
        ${target_name}
        BASE_NAME "${ARGS_BASE_NAME}"
        EXPORT_FILE_NAME "${_file_path}"
    )

    target_sources(
        ${target_name}
        PUBLIC
            FILE_SET public_headers
            TYPE HEADERS
            BASE_DIRS
                "${_base_dir}"
            FILES
                "${_file_path}"
    )
endfunction()

function(melt_install target_name)
    install(
        TARGETS ${target_name}
        RUNTIME
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT Runtime
        LIBRARY
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Runtime
        ARCHIVE
            DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT Development
        FILE_SET public_headers
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT Development
        FILE_SET interface_headers
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            COMPONENT Development
    )
endfunction()
