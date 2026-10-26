#include <entity/ecs/entity_manager.hpp>

#include <entity/ecs/entity.hpp>

#include <doctest/doctest.h>

#include <set>
#include <utility>
#include <vector>

#include <cstddef>

using namespace entity::ecs;

namespace {

/// \brief Checks that the given entity matches the expected one and is valid.
void checkEntityValid(
    const EntityManager& manager,
    Entity actual,
    Entity expected
) {
  CHECK(actual.id() == expected.id());
  CHECK(actual.version() == expected.version());
  CHECK(manager.isValid(actual));
}

/// \brief Requires that the given entity matches the expected one and is valid.
void requireEntityValid(
    const EntityManager& manager,
    Entity actual,
    Entity expected
) {
  REQUIRE(actual.id() == expected.id());
  REQUIRE(actual.version() == expected.version());
  REQUIRE(manager.isValid(actual));
}

}  // namespace

TEST_SUITE("EntityManager") {
  // --- EntityManager White-box ---

  TEST_CASE("Entities get unique sequential IDs") {
    // GIVEN
    EntityManager manager;

    // WHEN
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();
    const Entity e3 = manager.create();

    // THEN
    CHECK(e1.id() == 0);
    CHECK(e2.id() == 1);
    CHECK(e3.id() == 2);
  }

  TEST_CASE("New entities are created with version 0") {
    // GIVEN
    EntityManager manager;

    // WHEN
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();

    // THEN
    checkEntityValid(manager, e1, Entity{0, 0});
    checkEntityValid(manager, e2, Entity{1, 0});
  }

  TEST_CASE("Unallocated entities are invalid") {
    // GIVEN
    EntityManager manager;

    // THEN: Default and arbitrary uncreated entities are invalid
    CHECK_FALSE(manager.isValid(Entity{0, 0}));
    CHECK_FALSE(manager.isValid(Entity{0, 1}));
    CHECK_FALSE(manager.isValid(Entity{999, 0}));

    // WHEN
    const Entity e0 = manager.create();
    REQUIRE(manager.isValid(e0));

    // THEN: Next unallocated ID remains invalid
    CHECK_FALSE(manager.isValid(Entity{1, 0}));
  }

  TEST_CASE("Destroying an unallocated entity is a no-op") {
    // GIVEN
    EntityManager manager;

    // WHEN: Attempting to destroy entities that were never created
    manager.destroy(Entity{0, 0});
    manager.destroy(Entity{4, 2});

    // THEN: First created entity still starts at ID 0 and version 0
    Entity e0 = manager.create();
    checkEntityValid(manager, e0, Entity{0, 0});
  }

  TEST_CASE("Destruction increments version and recycles ID") {
    // GIVEN
    EntityManager manager;

    const Entity original = manager.create();
    requireEntityValid(manager, original, Entity{0, 0});

    // WHEN
    manager.destroy(original);

    // THEN
    CHECK_FALSE(manager.isValid(original));

    // WHEN
    const Entity recycled = manager.create();

    // THEN
    checkEntityValid(manager, recycled, Entity{0, 1});
  }

  TEST_CASE("Double destruction of the same entity is a no-op") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();

    // WHEN: The same entity is destroyed twice
    // First destroy is valid, increments internal version
    manager.destroy(entity);
    // Second destroy on the same entity should do nothing
    manager.destroy(entity);

    // THEN: ID 0 is pushed to the freelist only once
    const Entity recycled = manager.create();
    checkEntityValid(manager, recycled, Entity{0, 1});

    const Entity next = manager.create();
    checkEntityValid(manager, next, Entity{1, 0});
  }

  TEST_CASE("Forged entity with upcoming version is invalid while ID is free") {
    // GIVEN
    EntityManager manager;

    const Entity original = manager.create();
    manager.destroy(original);

    // WHEN: Forging an entity with the next expected version
    const Entity forged{0, 1};

    // THEN
    CHECK_FALSE(manager.isValid(forged));

    // WHEN: Attempting to destroy the forged entity
    manager.destroy(forged);

    // THEN: ID 0 is not duplicated in the freelist
    const Entity recycled = manager.create();
    const Entity next = manager.create();
    CHECK(recycled.id() == 0);
    CHECK(next.id() == 1);
  }

  TEST_CASE("Stale or future version does not affect live entity") {
    // GIVEN: Entity at ID 0 cycled to version 1
    EntityManager manager;

    const Entity stale_v0 = manager.create();
    manager.destroy(stale_v0);

    const Entity live_v1 = manager.create();
    requireEntityValid(manager, live_v1, Entity{0, 1});

    const Entity future_v2{0, 2};

    // WHEN: Destroying stale and future versions
    REQUIRE_FALSE(manager.isValid(stale_v0));
    REQUIRE_FALSE(manager.isValid(future_v2));

    manager.destroy(stale_v0);
    manager.destroy(future_v2);

    // THEN
    CHECK(manager.isValid(live_v1));
  }

  TEST_CASE("Retires slot permanently after exhausting all versions") {
    // GIVEN
    EntityManager manager;

    // WHEN: Cycling the same entity slot through all versions
    for (EntityType v = 0; v <= Entity::kVersionMax; ++v) {
      const Entity entity = manager.create();
      requireEntityValid(manager, entity, Entity{0, v});
      manager.destroy(entity);
    }

    // THEN: Slot is retired and never reused
    const Entity next = manager.create();
    checkEntityValid(manager, next, Entity{1, 0});

    CHECK_FALSE(manager.isValid(Entity{0, 0}));
    CHECK_FALSE(manager.isValid(Entity{0, Entity::kVersionMax}));
  }

  TEST_CASE("Move operations preserve manager state") {
    // GIVEN
    EntityManager original;
    Entity e0 = original.create();
    Entity e1 = original.create();
    original.destroy(e0);

    SUBCASE("Move construction transfers state") {
      // WHEN
      EntityManager moved(std::move(original));

      // THEN
      CHECK_FALSE(moved.isValid(e0));
      CHECK(moved.isValid(e1));

      const Entity recycled = moved.create();
      checkEntityValid(moved, recycled, Entity{0, 1});
    }

    SUBCASE("Move assignment transfers state") {
      // WHEN
      EntityManager moved;
      moved = std::move(original);

      // THEN
      CHECK_FALSE(moved.isValid(e0));
      CHECK(moved.isValid(e1));

      const Entity recycled = moved.create();
      checkEntityValid(moved, recycled, Entity{0, 1});
    }
  }

  // --- EntityManager Black-box ---

  TEST_CASE("Entities maintain unique IDs across creation and recycling") {
    // GIVEN
    EntityManager manager;

    std::set<EntityType> active_ids;
    std::vector<Entity> created_entities;

    const std::size_t initial_count = 100;
    const std::size_t destroy_count = 50;

    // WHEN: Creating the initial batch of entities
    for (std::size_t i = 0; i < initial_count; ++i) {
      const Entity entity = manager.create();
      created_entities.push_back(entity);
      active_ids.insert(entity.id());
    }

    // THEN
    REQUIRE(active_ids.size() == initial_count);

    // WHEN: Destroying some entities
    for (std::size_t i = 0; i < destroy_count; ++i) {
      const Entity entity_to_destroy = created_entities[i];
      manager.destroy(entity_to_destroy);
      active_ids.erase(entity_to_destroy.id());
    }

    // THEN
    REQUIRE(active_ids.size() == initial_count - destroy_count);

    // WHEN: Creating new entities to fill the recycled slots
    for (std::size_t i = 0; i < destroy_count; ++i) {
      const Entity entity = manager.create();
      active_ids.insert(entity.id());
    }

    // THEN: All active entities have unique raw IDs
    CHECK(active_ids.size() == initial_count);
  }

}  // TEST_SUITE("EntityManager")
