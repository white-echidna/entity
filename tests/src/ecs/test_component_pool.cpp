#include <entity/ecs/component_pool.hpp>

#include <entity/ecs/entity.hpp>
#include <entity/ecs/entity_manager.hpp>
#include <entity/strong_alias.hpp>

#include <glm/vec3.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace entity::ecs;
using entity::StrongAlias;

TEST_SUITE("ComponentPool") {
  struct VelocityTag;
  using Velocity = StrongAlias<VelocityTag, glm::vec3>;

  struct PositionTag;
  using Position = StrongAlias<PositionTag, glm::vec3>;

  struct AccelerationTag;
  using Acceleration = StrongAlias<AccelerationTag, glm::vec3>;

  // --- Family ---

  TEST_CASE("Family generates stable and unique IDs per type") {
    // WHEN
    const auto pos_id_1 = Family::id<Position>();
    const auto pos_id_2 = Family::id<Position>();
    const auto vel_id = Family::id<Velocity>();

    // THEN
    CHECK(pos_id_1 == pos_id_2);
    CHECK(pos_id_1 != vel_id);
  }

  // --- StorageCell ---

  TEST_CASE("StorageCell lazily initializes storage and validates types") {
    // GIVEN
    StorageCell cell;
    const StorageCell& const_cell = cell;

    EntityManager manager;
    const Entity entity = manager.create();
    const auto position = glm::vec3(1.F, 2.F, 3.F);

    // THEN: Uninitialized cell returns nullptr on safe getters
    CHECK(cell.tryGet<Position>() == nullptr);
    CHECK(const_cell.tryGet<Position>() == nullptr);

    // WHEN: Storage is lazily initialized
    auto& storage = cell.get<Position>();
    storage.emplace(entity, position);

    // THEN: Matching type succeeds, mismatched type returns nullptr
    REQUIRE(cell.tryGet<Position>() != nullptr);
    CHECK_EQ(cell.get<Position>().get(entity).value(), position);

    REQUIRE(const_cell.tryGet<Position>() != nullptr);
    CHECK_EQ(const_cell.get<Position>().get(entity).value(), position);

    CHECK(cell.tryGet<Velocity>() == nullptr);
    CHECK(const_cell.tryGet<Velocity>() == nullptr);
  }

  // --- ComponentPool ---

  TEST_CASE("Emplaces and retrieves a single component") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const auto position = glm::vec3(10.F, 20.F, 30.F);

    ComponentPool pool;

    // WHEN
    auto& emplaced = pool.emplace<Position>(entity, position);

    // THEN
    REQUIRE(pool.has<Position>(entity));
    CHECK_EQ(emplaced.value(), position);

    const auto& fetched = pool.get<Position>(entity);
    CHECK_EQ(fetched.value(), position);
  }

  TEST_CASE("tryGet safely handles existing and non-existent components") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const auto position = glm::vec3(10.F, 20.F, 30.F);

    ComponentPool pool;

    pool.emplace<Position>(entity, position);

    // WHEN / THEN: Existing component
    const auto* pos_ptr = pool.tryGet<Position>(entity);

    REQUIRE(pos_ptr != nullptr);
    CHECK_EQ(pos_ptr->value(), position);

    // WHEN / THEN: Non-existent component
    const auto* vel_ptr = pool.tryGet<Velocity>(entity);

    CHECK(vel_ptr == nullptr);
  }

  TEST_CASE("Handles multiple component types for a single entity") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();

    const auto position = glm::vec3(10.F, 20.F, 30.F);
    const auto velocity = glm::vec3(40.F, 50.F, 60.F);

    ComponentPool pool;

    // WHEN
    pool.emplace<Position>(entity, position);
    pool.emplace<Velocity>(entity, velocity);

    // THEN
    REQUIRE(pool.has<Position>(entity));
    REQUIRE(pool.has<Velocity>(entity));
    CHECK_EQ(pool.get<Position>(entity).value(), position);
    CHECK_EQ(pool.get<Velocity>(entity).value(), velocity);
  }

  TEST_CASE("erase removes a component from an entity") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const auto position = glm::vec3(10.F, 20.F, 30.F);

    ComponentPool pool;

    pool.emplace<Position>(entity, position);
    REQUIRE(pool.has<Position>(entity));

    // WHEN
    const bool erased = pool.erase<Position>(entity);

    // THEN
    CHECK(erased);
    CHECK_FALSE(pool.has<Position>(entity));
    CHECK(pool.tryGet<Position>(entity) == nullptr);
  }

  TEST_CASE(
      "entityDestroyed removes all components only for the target entity"
  ) {
    // GIVEN
    EntityManager manager;

    const Entity e1 = manager.create();
    const Entity e2 = manager.create();

    const auto position = glm::vec3(10.F, 20.F, 30.F);
    const auto velocity = glm::vec3(40.F, 50.F, 60.F);

    ComponentPool pool;

    pool.emplace<Position>(e1, position);
    pool.emplace<Velocity>(e1, velocity);

    pool.emplace<Position>(e2, position);
    pool.emplace<Velocity>(e2, velocity);

    REQUIRE(pool.has<Position>(e1));
    REQUIRE(pool.has<Velocity>(e1));

    REQUIRE(pool.has<Position>(e2));
    REQUIRE(pool.has<Velocity>(e2));

    // WHEN
    pool.entityDestroyed(e1);

    // THEN
    CHECK_FALSE(pool.has<Position>(e1));
    CHECK_FALSE(pool.has<Velocity>(e1));

    CHECK(pool.has<Position>(e2));
    CHECK(pool.has<Velocity>(e2));
  }

  TEST_CASE("clear removes all components and resets storages") {
    // GIVEN
    EntityManager manager;

    const Entity e1 = manager.create();
    const Entity e2 = manager.create();
    const Entity e3 = manager.create();

    const auto position = glm::vec3(1.F, 2.F, 3.F);
    const auto velocity = glm::vec3(4.F, 5.F, 6.F);
    const auto acceleration = glm::vec3(7.F, 8.F, 9.F);

    ComponentPool pool;

    pool.emplace<Position>(e1, position);
    pool.emplace<Velocity>(e2, velocity);

    REQUIRE(pool.has<Position>(e1));
    REQUIRE(pool.has<Velocity>(e2));

    // WHEN
    pool.clear();

    // THEN
    CHECK_FALSE(pool.has<Position>(e1));
    CHECK_FALSE(pool.has<Velocity>(e2));

    CHECK(pool.tryGet<Position>(e1) == nullptr);
    CHECK(pool.tryGet<Velocity>(e2) == nullptr);

    CHECK(pool.view<Position>().empty());

    // WHEN: Re-emplacing after clear
    pool.emplace<Acceleration>(e3, acceleration);

    // THEN
    REQUIRE(pool.has<Acceleration>(e3));
    CHECK_EQ(pool.get<Acceleration>(e3).value(), acceleration);
  }

  TEST_CASE("view provides read-only access to component storage") {
    // GIVEN
    EntityManager manager;

    const Entity e1 = manager.create();
    const Entity e2 = manager.create();

    const auto position1 = glm::vec3(1.F, 2.F, 3.F);
    const auto position2 = glm::vec3(4.F, 5.F, 6.F);

    ComponentPool pool;

    SUBCASE("Returns empty view when storage does not exist") {
      // WHEN
      auto empty_view = pool.view<Acceleration>();

      // THEN
      CHECK(empty_view.empty());
      CHECK(empty_view.size() == 0);
      CHECK(empty_view.begin() == empty_view.end());
      CHECK(empty_view.tryGet(e1) == nullptr);
    }

    SUBCASE("Returns populated view when storage exists") {
      // WHEN
      pool.emplace<Position>(e1, position1);
      pool.emplace<Position>(e2, position2);

      auto pos_view = pool.view<Position>();

      // THEN
      REQUIRE_FALSE(pos_view.empty());
      REQUIRE(pos_view.size() == 2);

      CHECK_EQ(pos_view.get(e1).value(), position1);
      CHECK_EQ(pos_view.get(e2).value(), position2);

      std::vector<Entity> visited(pos_view.begin(), pos_view.end());
      REQUIRE(visited.size() == 2);

      CHECK(visited[0] == e1);
      CHECK(visited[1] == e2);
    }
  }

  TEST_CASE("Const-correctness is enforced") {
    // GIVEN
    EntityManager manager;
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();
    const auto position = glm::vec3(10.F, 20.F, 30.F);

    ComponentPool pool;
    pool.emplace<Position>(e1, position);
    REQUIRE(pool.has<Position>(e1));

    const ComponentPool& const_pool = pool;

    SUBCASE("has works on a const pool") {
      CHECK(const_pool.has<Position>(e1));
      CHECK_FALSE(const_pool.has<Velocity>(e1));
      CHECK_FALSE(const_pool.has<Position>(e2));
    }

    SUBCASE("get returns a const reference on a const pool") {
      const auto& pos_ref = const_pool.get<Position>(e1);
      CHECK_EQ(pos_ref.value(), position);

      // This line should fail to compile, proving const-correctness:
      // pos_ref.value() = glm::vec3(40, 50, 60);
    }

    SUBCASE("tryGet returns a const pointer on a const pool") {
      const auto* pos_ptr = const_pool.tryGet<Position>(e1);
      REQUIRE(pos_ptr != nullptr);
      CHECK_EQ(pos_ptr->value(), position);

      CHECK(const_pool.tryGet<Velocity>(e1) == nullptr);
      CHECK(const_pool.tryGet<Position>(e2) == nullptr);

      // This line should fail to compile, proving const-correctness:
      // pos_ptr->value() = glm::vec3(40, 50, 60);
    }

    SUBCASE("Non-const methods cannot be called on a const pool") {
      // This line should fail to compile, proving const-correctness:
      // const_pool.emplace<Velocity>(e1, glm::vec3(40.F, 50.F, 60.F));

      // This line should fail to compile, proving const-correctness:
      // const_pool.erase<Position>(e1);

      // This line should fail to compile, proving const-correctness:
      // const_pool.clear();
    }
  }

}  // TEST_SUITE("ComponentPool")
