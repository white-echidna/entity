
find_package(doctest CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(ENTITY_TESTS_SOURCES
    "src/ecs/test_component_pool.cpp"
    "src/ecs/test_entity_manager.cpp"
    "src/ecs/test_sparse_set.cpp"
    "src/ecs/test_storage.cpp"
    "src/ecs/test_view.cpp"
)

add_executable(entity_tests)

melt_sources(
    entity_tests
    PRIVATE
        "src/test_main.cpp"
        ${ENTITY_TESTS_SOURCES}
)

target_link_libraries(
    entity_tests
    PRIVATE
        entity_compile_options

        entity
        entity_private_headers

        doctest::doctest
        glm::glm
)

include(doctest)
doctest_discover_tests(entity_tests)
