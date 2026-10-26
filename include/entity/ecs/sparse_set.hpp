#pragma once

#ifndef ENTITY_ECS_SPARSE_SET_HPP
#define ENTITY_ECS_SPARSE_SET_HPP

#include <entity/ecs/entity.hpp>

#include <concepts>
#include <iterator>
#include <limits>
#include <utility>
#include <vector>

#include <cassert>
#include <cstddef>

namespace entity::ecs {

// A compact optional-like wrapper for unsigned integral indices.
// Avoids the memory overhead of `std::optional` by reserving the sentinel value
// to represent the null state.
template <std::unsigned_integral T>
class OptionalIndex {
 public:
  static constexpr T kSentinel = std::numeric_limits<T>::max();

  // --- Constructors ---

  // Constructs a null index.
  constexpr OptionalIndex() noexcept
      : m_value(kSentinel) {}

  // Constructs an index holding `index`.
  // Precondition: `index != kSentinel`.
  constexpr explicit OptionalIndex(T index) noexcept
      : m_value(index) {
    assert(index != kSentinel);
  }

  // --- Observers ---

  [[nodiscard]] constexpr auto hasValue() const noexcept -> bool {
    return m_value != kSentinel;
  }

  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return hasValue();
  }

  // --- Element Access ---

  // Returns the stored index.
  // Precondition: holding non-null value.
  [[nodiscard]] constexpr auto value() const noexcept -> T {
    assert(hasValue());
    return m_value;
  }

 private:
  T m_value;
};

// Manages a set of entities using a sparse array for lookups by entity ID
// and a tightly packed dense array for contiguous iteration.
class SparseSet {
 public:
  // --- Standard Container Types ---

  using value_type = Entity;
  using size_type = std::vector<value_type>::size_type;
  using const_iterator = std::vector<value_type>::const_iterator;
  using iterator = const_iterator;

  // --- Modifiers ---

  // Inserts `entity` into the set if it is not already present.
  // Returns an iterator to the entity and true if insertion took place.
  auto insert(value_type entity) -> std::pair<const_iterator, bool> {
    if (const auto it = find(entity); it != cend()) {
      return {it, false};
    }
    const auto it = insertUnchecked(entity);
    return {it, true};
  }

  // Removes `entity` from the set using swap-and-pop.
  // Returns true if the entity was present and removed.
  auto erase(value_type entity) noexcept -> bool {
    if (!contains(entity)) {
      return false;
    }

    // --- Swap & Pop ---
    const auto id_to_delete = entity.id();
    const auto index_to_delete = m_sparse[id_to_delete].value();

    const auto last_entity_in_dense = m_dense_entities.back();
    const auto last_entity_id = last_entity_in_dense.id();

    // 1. Move the last element into the place of the one being removed.
    m_dense_entities[index_to_delete] = last_entity_in_dense;
    // 2. Update the sparse entry for the moved entity to point to its new loc.
    m_sparse[last_entity_id] = OptionalIndex(index_to_delete);
    // 3. Mark the sparse entry for the deleted entity as null.
    m_sparse[id_to_delete] = OptionalIndex<size_type>();
    // 4. Pop the now-duplicate last element from the dense array.
    m_dense_entities.pop_back();

    return true;
  }

  // Removes all entities from the set.
  void clear() noexcept {
    m_dense_entities.clear();
    m_sparse.clear();
  }

  // --- Lookup ---

  // Returns an iterator to `entity` in the dense array, or `cend()` if not found.
  [[nodiscard]] auto find(value_type entity) const noexcept -> const_iterator {
    if (contains(entity)) {
      const auto offset = static_cast<std::ptrdiff_t>(index(entity));
      return m_dense_entities.cbegin() + offset;
    }
    return m_dense_entities.cend();
  }

  // Returns true if `entity` is in the set.
  [[nodiscard]] auto contains(value_type entity) const noexcept -> bool {
    const auto id = entity.id();
    const auto version = entity.version();

    // An entity is in the set if:
    // 1. Its ID is within the bounds of the sparse array.
    // 2. Its entry in the sparse array is not null.
    // 3. The entity version in the dense array at the stored index
    //    matches the one we are looking for.
    return id < m_sparse.size() && m_sparse[id].hasValue() &&
           m_dense_entities[m_sparse[id].value()].version() == version;
  }

  // --- Direct Data Access ---

  // Returns the dense array index of `entity`.
  // Precondition: `contains(entity)` is true.
  [[nodiscard]] auto index(value_type entity) const noexcept -> size_type {
    assert(contains(entity) && "Entity does not exist in the set.");
    return m_sparse[entity.id()].value();
  }

  // Returns a read-only reference to the underlying dense array of entities.
  [[nodiscard]] auto dense() const noexcept -> const std::vector<value_type>& {
    return m_dense_entities;
  }

  // --- Iterators ---

  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    return m_dense_entities.begin();
  }

  [[nodiscard]] auto end() const noexcept -> const_iterator {
    return m_dense_entities.end();
  }

  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return m_dense_entities.cbegin();
  }

  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return m_dense_entities.cend();
  }

  // --- Capacity ---

  [[nodiscard]] auto empty() const noexcept -> bool {
    return m_dense_entities.empty();
  }

  [[nodiscard]] auto size() const noexcept -> size_type {
    return m_dense_entities.size();
  }

 private:
  // Appends `entity` to the dense array and records its index in the sparse array.
  // Precondition: The sparse slot for `entity.id()` is not occupied.
  auto insertUnchecked(value_type entity) -> const_iterator {
    const auto id = entity.id();
    if (id >= m_sparse.size()) {
      m_sparse.resize(id + 1);
    }

    assert(
        !m_sparse[id].hasValue() &&
        "Entity ID slot is already occupied by another version."
    );

    m_sparse[id] = OptionalIndex(m_dense_entities.size());
    m_dense_entities.push_back(entity);

    return std::prev(m_dense_entities.cend());
  }

  std::vector<value_type> m_dense_entities;
  std::vector<OptionalIndex<size_type>> m_sparse;
};

}  // namespace entity::ecs

#endif  // ENTITY_ECS_SPARSE_SET_HPP
