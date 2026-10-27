cmake_minimum_required(VERSION 4.0)
project(entity LANGUAGES CXX)

option(ENTITY_BUILD_TESTS "Build tests for entity" OFF)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/doctest/scripts/cmake"
)

include(ProjectHelpers)

enable_testing()

set(ENTITY_PUBLIC_SOURCES
    "include/entity/strong_alias.hpp"

    "include/entity/ecs/entity.hpp"
    "include/entity/ecs/entity_manager.hpp"
    "include/entity/ecs/sparse_set.hpp"
    "include/entity/ecs/storage.hpp"
)

set(ENTITY_PRIVATE_SOURCES
    "src/entity_manager.cpp"
)

add_library(entity STATIC)

melt_sources(
    entity
    PUBLIC
        ${ENTITY_PUBLIC_SOURCES}
    PRIVATE
        ${ENTITY_PRIVATE_SOURCES}
)

melt_export_header(
    entity
    BASE_NAME ENTITY
    FILE_NAME "entity/export.hpp"
)

add_library(entity_private_headers INTERFACE)

target_include_directories(
    entity_private_headers
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(
    entity
    PRIVATE
        entity_compile_options
        entity_private_headers
)

melt_install(entity)

if (ENTITY_BUILD_TESTS)
    add_subdirectory(tests)
endif()
