#include <entity/ecs/storage.hpp>

#include <entity/ecs/entity.hpp>
#include <entity/ecs/entity_manager.hpp>

#include <glm/vec3.hpp>

#include <doctest/doctest.h>

#include <array>
#include <memory>
#include <utility>

#include <cstddef>

using namespace entity::ecs;

namespace {

template <typename T>
class MoveOnlyComponent {
 public:
  // --- Constructors ---

  explicit MoveOnlyComponent(T v)
      : m_data(std::make_unique<T>(std::move(v))) {}

  // --- Rule of Five ---

  ~MoveOnlyComponent() = default;

  MoveOnlyComponent(const MoveOnlyComponent&) = delete;
  auto operator=(const MoveOnlyComponent&) -> MoveOnlyComponent& = delete;

  MoveOnlyComponent(MoveOnlyComponent&& other) noexcept = default;
  auto operator=(MoveOnlyComponent&& other) noexcept -> MoveOnlyComponent& {
    REQUIRE_MESSAGE(
        this != &other,
        "Self-move-assignment detected in Storage::erase"
    );
    m_data = std::move(other.m_data);
    return *this;
  }

  // --- Lookup ---

  [[nodiscard]] auto get() -> std::unique_ptr<T>& {
    return m_data;
  }

  [[nodiscard]] auto get() const -> const std::unique_ptr<T>& {
    return m_data;
  }

 private:
  std::unique_ptr<T> m_data;
};

}  // namespace

TEST_SUITE("Storage") {
  // --- Storage ---

  TEST_CASE("Is empty on creation") {
    // GIVEN
    Storage<glm::vec3> storage;

    // THEN
    CHECK(storage.empty());
    CHECK(storage.size() == 0);
    CHECK(storage.begin() == storage.end());
  }

  TEST_CASE("emplace constructs and returns a component") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const auto expected = glm::vec3(1.F, 1.F, 1.F);
    Storage<glm::vec3> storage;

    // WHEN
    auto& actual = storage.emplace(entity, expected);

    // THEN
    CHECK(storage.size() == 1);
    CHECK_FALSE(storage.empty());
    CHECK(storage.contains(entity));
    CHECK_EQ(actual, expected);
  }

  TEST_CASE("get retrieves the component for an entity") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const auto expected = glm::vec3(1.F, 1.F, 1.F);

    Storage<glm::vec3> storage;
    const Storage<glm::vec3>& const_storage = storage;

    storage.emplace(entity, expected);

    // WHEN / THEN
    CHECK_EQ(storage.get(entity), expected);
    CHECK_EQ(const_storage.get(entity), expected);
  }

  TEST_CASE("tryGet safely returns a pointer or nullptr") {
    // GIVEN
    EntityManager manager;
    const Entity entity = manager.create();
    const Entity missing_entity = manager.create();
    const auto expected = glm::vec3(1.F, 1.F, 1.F);

    Storage<glm::vec3> storage;
    const Storage<glm::vec3>& const_storage = storage;

    storage.emplace(entity, expected);

    REQUIRE(storage.contains(entity));
    REQUIRE_FALSE(storage.contains(missing_entity));

    // WHEN
    auto* found_component = storage.tryGet(entity);
    const auto* const_found_component = const_storage.tryGet(entity);

    // THEN
    REQUIRE(found_component != nullptr);
    CHECK_EQ(*found_component, expected);

    REQUIRE(const_found_component != nullptr);
    CHECK_EQ(*const_found_component, expected);

    CHECK(storage.tryGet(missing_entity) == nullptr);
    CHECK(const_storage.tryGet(missing_entity) == nullptr);
  }

  TEST_CASE("erase removes a component via swap-and-pop") {
    // GIVEN
    EntityManager manager;
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();
    const Entity e3 = manager.create();

    const auto c1 = glm::vec3(1.F, 1.F, 1.F);
    const auto c2 = glm::vec3(2.F, 2.F, 2.F);
    const auto c3 = glm::vec3(3.F, 3.F, 3.F);

    Storage<glm::vec3> storage;
    storage.emplace(e1, c1);
    storage.emplace(e2, c2);
    storage.emplace(e3, c3);

    REQUIRE(storage.size() == 3);

    // WHEN
    const bool erased = storage.erase(e2);

    // THEN
    REQUIRE(erased);
    CHECK(storage.size() == 2);

    CHECK(storage.contains(e1));
    CHECK_FALSE(storage.contains(e2));
    CHECK(storage.contains(e3));

    // THEN: The last element's data moved into the erased middle slot
    const auto& data = storage.components();
    CHECK_EQ(data[0], c1);
    CHECK_EQ(data[1], c3);
  }

  TEST_CASE("Supports move-only components") {
    // GIVEN
    EntityManager manager;
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();

    Storage<MoveOnlyComponent<int>> storage;
    storage.emplace(e1, 2);
    storage.emplace(e2, 3);

    // WHEN: Erasing the last element in dense storage
    REQUIRE(storage.erase(e2));

    // THEN: Remaining entity stays valid and intact
    CHECK(storage.size() == 1);

    REQUIRE(storage.get(e1).get() != nullptr);
    CHECK(*storage.get(e1).get() == 2);

    // WHEN: Erasing the sole remaining element
    REQUIRE(storage.erase(e1));

    // THEN
    CHECK(storage.empty());
  }

  TEST_CASE("Component and entity arrays remain synchronized after erase") {
    // GIVEN
    EntityManager manager;

    const std::array<Entity, 4> initial_entities = {
        manager.create(),
        manager.create(),
        manager.create(),
        manager.create(),
    };

    const std::array<glm::vec3, 4> initial_components = {
        glm::vec3(1.F, 1.F, 1.F),
        glm::vec3(2.F, 2.F, 2.F),
        glm::vec3(3.F, 3.F, 3.F),
        glm::vec3(4.F, 4.F, 4.F),
    };

    Storage<glm::vec3> storage;
    for (std::size_t i = 0; i < initial_entities.size(); ++i) {
      storage.emplace(initial_entities.at(i), initial_components.at(i));
    }

    REQUIRE(storage.size() == 4);

    // WHEN: An element from the middle is erased
    REQUIRE(storage.erase(initial_entities[1]));
    REQUIRE(storage.size() == 3);

    // THEN: Dense arrays of components and entities remain 1-to-1 synchronized
    const auto& entities = storage.entities();
    const auto& components = storage.components();

    REQUIRE(entities.size() == 3);
    REQUIRE(components.size() == 3);

    for (std::size_t i = 0; i < storage.size(); ++i) {
      INFO("Checking sync at dense index ", i);
      CHECK_EQ(storage.get(entities[i]), components[i]);
    }
  }

  TEST_CASE("clear erases all components") {
    // GIVEN
    EntityManager manager;
    const Entity e1 = manager.create();
    const Entity e2 = manager.create();

    const auto c1 = glm::vec3(1, 1, 1);
    const auto c2 = glm::vec3(2, 2, 2);

    Storage<glm::vec3> storage;
    storage.emplace(e1, c1);
    storage.emplace(e2, c2);

    REQUIRE_FALSE(storage.empty());
    REQUIRE(storage.size() == 2);

    // WHEN
    storage.clear();

    // THEN
    CHECK(storage.empty());
    CHECK(storage.size() == 0);
    CHECK(storage.begin() == storage.end());

    CHECK_FALSE(storage.contains(e1));
    CHECK_FALSE(storage.contains(e2));
  }

}  // TEST_SUITE("Storage")
