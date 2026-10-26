
find_package(doctest REQUIRED)

set(ENTITY_TESTS_SOURCES
    # Entity Component System
    "src/ecs/test_entity_manager.cpp"
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
)

include(doctest)
doctest_discover_tests(entity_tests)
