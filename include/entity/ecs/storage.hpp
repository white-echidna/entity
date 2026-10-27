#pragma once

#ifndef ENTITY_ECS_STORAGE_HPP
#define ENTITY_ECS_STORAGE_HPP

#include <entity/ecs/entity.hpp>
#include <entity/ecs/sparse_set.hpp>

#include <concepts>
#include <type_traits>
#include <utility>
#include <vector>

namespace entity::ecs {

// Type-erased base interface for component storages.
class IStorage {
 public:
  IStorage() = default;

  virtual ~IStorage() = default;

  IStorage(const IStorage&) = delete;
  auto operator=(const IStorage&) -> IStorage& = delete;

  IStorage(IStorage&&) noexcept = delete;
  auto operator=(IStorage&&) noexcept -> IStorage& = delete;

  virtual auto erase(Entity entity) noexcept -> bool = 0;
};

// Associates components of type `T` with entities in a tightly packed dense array
// synchronized 1-to-1 with an underlying `SparseSet`.
template <std::movable T>
class Storage : public IStorage, private SparseSet {
  static_assert(
      std::is_nothrow_move_constructible_v<T>,
      "Components must be nothrow move-constructible."
  );
  static_assert(
      std::is_nothrow_move_assignable_v<T>,
      "Components must be nothrow move-assignable."
  );

 public:
  // --- Standard Container Types ---

  using value_type = T;
  using entity_type = SparseSet::value_type;
  using size_type = SparseSet::size_type;
  using iterator = std::vector<value_type>::iterator;
  using const_iterator = std::vector<value_type>::const_iterator;

  // Expose using-declarations from the private base class
  using SparseSet::contains;
  using SparseSet::empty;
  using SparseSet::index;
  using SparseSet::size;

  // --- Modifiers ---

  // Constructs a component in-place for `entity` if absent, or returns the existing one.
  template <typename... Args>
  auto emplace(entity_type entity, Args&&... args) -> value_type& {
    const auto result = SparseSet::insert(entity);

    if (!result.second) {
      return get(entity);  // Component already exists for this entity.
    }

    return m_data.emplace_back(std::forward<Args>(args)...);
  }

  // Removes the component associated with `entity` using swap-and-pop.
  // Returns true if the component was present and removed.
  auto erase(entity_type entity) noexcept -> bool override {
    if (!contains(entity)) {
      return false;
    }

    const auto index_to_delete = SparseSet::index(entity);
    const auto last_index = m_data.size() - 1;

    if (index_to_delete != last_index) {
      m_data[index_to_delete] = std::move(m_data.back());
    }

    m_data.pop_back();
    SparseSet::erase(entity);

    return true;
  }

  // Removes all entities and components from the storage.
  void clear() noexcept {
    m_data.clear();
    SparseSet::clear();
  }

  // --- Lookup ---

  // Returns a reference to the component of `entity`.
  // Precondition: `contains(entity)` is true.
  [[nodiscard]]
  auto get(entity_type entity) noexcept -> value_type& {
    return m_data[SparseSet::index(entity)];
  }

  // Returns a const reference to the component of `entity`.
  // Precondition: `contains(entity)` is true.
  [[nodiscard]]
  auto get(entity_type entity) const noexcept -> const value_type& {
    return m_data[SparseSet::index(entity)];
  }

  // Returns a pointer to the component of `entity`, or `nullptr` if not found.
  [[nodiscard]]
  auto tryGet(entity_type entity) noexcept -> value_type* {
    return contains(entity) ? &get(entity) : nullptr;
  }

  // Returns a const pointer to the component of `entity`, or `nullptr` if not found.
  [[nodiscard]]
  auto tryGet(entity_type entity) const noexcept -> const value_type* {
    return contains(entity) ? &get(entity) : nullptr;
  }

  // --- Direct Data Access ---

  // Returns the dense array of components.
  [[nodiscard]]
  auto components() const noexcept -> const std::vector<value_type>& {
    return m_data;
  }

  // Returns the dense array of entities synchronized by index with `components()`.
  [[nodiscard]]
  auto entities() const noexcept -> const std::vector<entity_type>& {
    return SparseSet::dense();
  }

  // --- Iterators ---

  [[nodiscard]] auto begin() noexcept -> iterator {
    return m_data.begin();
  }

  [[nodiscard]] auto end() noexcept -> iterator {
    return m_data.end();
  }

  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return m_data.begin();
  }

  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return m_data.end();
  }

  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return m_data.cbegin();
  }

  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return m_data.cend();
  }

 private:
  std::vector<value_type> m_data;
};

}  // namespace entity::ecs

#endif  // ENTITY_ECS_STORAGE_HPP
