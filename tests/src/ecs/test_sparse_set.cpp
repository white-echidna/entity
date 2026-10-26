#include <entity/ecs/sparse_set.hpp>

#include <entity/ecs/entity.hpp>
#include <entity/ecs/entity_manager.hpp>

#include <doctest/doctest.h>

#include <vector>

using namespace entity::ecs;

TEST_SUITE("SparseSet") {
  // --- OptionalIndex ---

  TEST_CASE("OptionalIndex handles null and zero-based indices") {
    // GIVEN
    OptionalIndex<SparseSet::size_type> empty;

    // THEN
    CHECK_FALSE(empty.hasValue());
    CHECK_FALSE(static_cast<bool>(empty));

    // WHEN: Constructed with zero index
    OptionalIndex<SparseSet::size_type> zero(0);

    // THEN
    REQUIRE(zero.hasValue());
    CHECK(zero.value() == 0);

    // WHEN: Constructed with a non-zero index
    const SparseSet::size_type target_index = 42;
    OptionalIndex<SparseSet::size_type> valid(target_index);

    // THEN
    REQUIRE(valid.hasValue());
    CHECK(valid.value() == target_index);
  }

  // --- SparseSet ---

  TEST_CASE("Is empty on creation") {
    // GIVEN
    SparseSet set;

    // THEN
    CHECK(set.empty());
    CHECK(set.size() == 0);
    CHECK(set.begin() == set.end());
  }

  TEST_CASE("Inserts a new entity") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();

    SparseSet set;

    // WHEN
    auto [it, inserted] = set.insert(entity);

    // THEN
    CHECK(inserted);
    CHECK(set.contains(entity));

    REQUIRE(it != set.end());
    CHECK(*it == entity);
  }

  TEST_CASE("Rejects insertion of a duplicate entity") {
    // GIVEN
    EntityManager manager;
    auto entity = manager.create();

    SparseSet set;

    // WHEN
    set.insert(entity);
    REQUIRE(set.contains(entity));

    auto [it, inserted] = set.insert(entity);

    // THEN
    CHECK_FALSE(inserted);

    REQUIRE(it != set.end());
    CHECK(*it == entity);

    CHECK(set.size() == 1);
  }

  TEST_CASE("Handles sparse non-sequential entity IDs with gaps") {
    // GIVEN
    SparseSet set;

    const Entity high_id_entity{500, 3};
    const Entity low_id_entity{2, 1};

    // WHEN: Inserting an entity with a high ID first, then a low ID
    auto [it1, inserted1] = set.insert(high_id_entity);
    auto [it2, inserted2] = set.insert(low_id_entity);

    // THEN
    CHECK(inserted1);
    CHECK(inserted2);
    REQUIRE(set.size() == 2);

    CHECK(set.contains(high_id_entity));
    CHECK(set.index(high_id_entity) == 0);

    CHECK(set.contains(low_id_entity));
    CHECK(set.index(low_id_entity) == 1);

    // THEN: Intermediate and out-of-bounds IDs are not present
    CHECK_FALSE(set.contains(Entity{0, 0}));
    CHECK_FALSE(set.contains(Entity{499, 0}));
    CHECK_FALSE(set.contains(Entity{501, 0}));
  }

  TEST_CASE("find locates existing entities and returns end for missing ones") {
    // GIVEN
    SparseSet set;
    const SparseSet& const_set = set;

    const Entity e1{10, 0};
    const Entity e2{20, 1};

    set.insert(e1);
    set.insert(e2);

    SUBCASE("Returns valid iterator for existing entity") {
      // WHEN
      auto it = set.find(e1);
      auto const_it = const_set.find(e2);

      // THEN
      REQUIRE(it != set.end());
      CHECK(*it == e1);

      REQUIRE(const_it != const_set.end());
      CHECK(*const_it == e2);
    }

    SUBCASE("Returns end iterator for non-existent or mismatched entities") {
      // GIVEN
      const Entity missing_in_bounds{5, 0};
      const Entity missing_out_of_bounds{100, 0};
      const Entity wrong_version{10, 2};

      // THEN
      CHECK(set.find(missing_in_bounds) == set.end());
      CHECK(set.find(missing_out_of_bounds) == set.end());
      CHECK(set.find(wrong_version) == set.end());

      CHECK(const_set.find(missing_in_bounds) == const_set.end());
      CHECK(const_set.find(missing_out_of_bounds) == const_set.end());
      CHECK(const_set.find(wrong_version) == const_set.end());
    }
  }

  TEST_CASE("erase removes entities and maintains dense array invariants") {
    // GIVEN
    SparseSet set;

    const Entity e1{0, 0};
    const Entity e2{1, 0};
    const Entity e3{2, 0};

    SUBCASE("Returns false when erasing from an empty set") {
      CHECK_FALSE(set.erase(e1));
    }

    SUBCASE("Returns false when erasing an out-of-bounds entity") {
      set.insert(e1);
      set.insert(e2);

      CHECK_FALSE(set.erase(Entity{99, 0}));
      CHECK(set.size() == 2);
    }

    SUBCASE("Performs swap-and-pop when erasing a middle element") {
      // GIVEN
      set.insert(e1);
      set.insert(e2);
      set.insert(e3);

      REQUIRE(set.size() == 3);

      // WHEN
      const bool erased = set.erase(e2);

      // THEN: Last element takes the place of erased element
      REQUIRE(erased);
      REQUIRE(set.size() == 2);
      CHECK_FALSE(set.contains(e2));

      CHECK(set.dense()[0] == e1);
      CHECK(set.dense()[1] == e3);
      CHECK(set.index(e3) == 1);
    }

    SUBCASE("Erases the last and sole remaining elements cleanly") {
      // GIVEN
      set.insert(e1);
      set.insert(e2);

      // WHEN: Erasing the back element
      REQUIRE(set.erase(e2));

      // THEN
      CHECK_FALSE(set.contains(e2));
      CHECK(set.size() == 1);

      REQUIRE(set.contains(e1));
      CHECK(set.index(e1) == 0);

      // WHEN: Erasing the sole remaining element
      REQUIRE(set.erase(e1));

      // THEN
      CHECK_FALSE(set.contains(e1));
      CHECK(set.empty());
    }
  }

  TEST_CASE("Handles entity versioning across lifecycle") {
    // GIVEN
    EntityManager manager;
    auto old_entity = manager.create();
    manager.destroy(old_entity);
    auto new_entity = manager.create();

    REQUIRE(old_entity.id() == new_entity.id());
    REQUIRE(old_entity.version() < new_entity.version());

    SparseSet set;
    set.insert(old_entity);

    SUBCASE("Rejects lookup for mismatched version on active slot") {
      CHECK(set.contains(old_entity));
      CHECK_FALSE(set.contains(new_entity));
      CHECK(set.find(new_entity) == set.end());
      CHECK_FALSE(set.erase(new_entity));
    }

    SUBCASE("Allows inserting new version after old version is erased") {
      // WHEN
      REQUIRE(set.erase(old_entity));
      auto [it, inserted] = set.insert(new_entity);

      // THEN
      CHECK(inserted);
      CHECK(set.contains(new_entity));
      CHECK_FALSE(set.contains(old_entity));
    }
  }

  TEST_CASE("clear erases all entities") {
    // GIVEN
    EntityManager manager;
    const Entity e0 = manager.create();
    const Entity e1 = manager.create();

    SparseSet set;
    set.insert(e0);
    set.insert(e1);

    REQUIRE_FALSE(set.empty());
    REQUIRE(set.size() == 2);

    // WHEN
    set.clear();

    // THEN
    CHECK(set.empty());
    CHECK(set.size() == 0);
    CHECK(set.begin() == set.end());
    CHECK_FALSE(set.contains(e0));
    CHECK_FALSE(set.contains(e1));
  }

  TEST_CASE("Iterators traverse dense array in insertion order") {
    // GIVEN
    SparseSet set;

    const Entity e1{1, 0};
    const Entity e2{2, 0};
    const Entity e3{3, 0};

    set.insert(e1);
    set.insert(e2);
    set.insert(e3);

    SUBCASE("Mutable iteration matches dense array") {
      std::vector<Entity> visited(set.begin(), set.end());
      CHECK(visited == set.dense());
    }

    SUBCASE("Const iteration matches dense array") {
      const SparseSet& const_set = set;
      std::vector<Entity> visited(const_set.begin(), const_set.end());
      CHECK(visited == const_set.dense());
    }
  }

}  // TEST_SUITE("SparseSet")
