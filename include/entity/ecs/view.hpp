#pragma once

#ifndef ENTITY_ECS_VIEW_HPP
#define ENTITY_ECS_VIEW_HPP

#include <entity/ecs/entity.hpp>
#include <entity/ecs/sparse_set.hpp>
#include <entity/ecs/storage.hpp>

#include <span>

#include <cassert>

namespace entity::ecs {

// A non-owning read-only view over a single component storage `Storage<T>`.
template <typename T>
class View {
 public:
  // --- Standard Container Types ---

  using value_type = SparseSet::value_type;
  using component_type = Storage<T>::value_type;
  using size_type = SparseSet::size_type;
  using const_iterator = SparseSet::const_iterator;

  // --- Constructors ---

  constexpr View() noexcept = default;

  // Constructs a view from an existing storage pointer.
  constexpr explicit View(const Storage<T>* storage) noexcept
      : m_storage(storage) {}

  // --- Lookup ---

  // Returns true if `entity` is present in the viewed storage.
  [[nodiscard]] auto contains(value_type entity) const noexcept -> bool {
    return m_storage != nullptr && m_storage->contains(entity);
  }

  // Returns a const reference to the component of `entity`.
  // Precondition: `contains(entity)` is true.
  [[nodiscard]]
  auto get(value_type entity) const noexcept -> const component_type& {
    assert(m_storage && "Viewed storage is null.");
    return m_storage->get(entity);
  }

  // Returns a const pointer to the component of `entity`, or `nullptr` if not found.
  [[nodiscard]]
  auto tryGet(value_type entity) const noexcept -> const component_type* {
    if (m_storage == nullptr) {
      return nullptr;
    }
    return m_storage->tryGet(entity);
  }

  // --- Direct Data Access ---

  // Returns a read-only span over the dense array of components.
  [[nodiscard]]
  auto components() const noexcept -> std::span<const component_type> {
    if (m_storage == nullptr) {
      return {};
    }
    return m_storage->components();
  }

  // Returns a read-only span over the dense array of entities.
  [[nodiscard]]
  auto entities() const noexcept -> std::span<const value_type> {
    if (m_storage == nullptr) {
      return {};
    }
    return m_storage->entities();
  }

  // --- Iterators ---

  [[nodiscard]] auto begin() const noexcept -> const_iterator {
    if (m_storage == nullptr) {
      return const_iterator{};
    }
    return m_storage->entities().begin();
  }

  [[nodiscard]] auto end() const noexcept -> const_iterator {
    if (m_storage == nullptr) {
      return const_iterator{};
    }
    return m_storage->entities().end();
  }

  [[nodiscard]] auto cbegin() const noexcept -> const_iterator {
    return begin();
  }

  [[nodiscard]] auto cend() const noexcept -> const_iterator {
    return end();
  }

  // --- Capacity ---

  [[nodiscard]] auto empty() const noexcept -> bool {
    return m_storage == nullptr || m_storage->empty();
  }

  [[nodiscard]] constexpr auto size() const noexcept -> size_type {
    if (m_storage == nullptr) {
      return 0;
    }
    return m_storage->size();
  }

  /// \}

 private:
  const Storage<T>* m_storage = nullptr;
};

}  // namespace entity::ecs

#endif  // ENTITY_ECS_VIEW_HPP
