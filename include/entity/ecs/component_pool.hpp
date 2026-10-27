#pragma once

#ifndef ENTITY_ECS_COMPONENT_POOL_HPP
#define ENTITY_ECS_COMPONENT_POOL_HPP

#include <entity/ecs/entity.hpp>
#include <entity/ecs/sparse_set.hpp>
#include <entity/ecs/storage.hpp>
#include <entity/ecs/view.hpp>

#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include <cassert>
#include <cstddef>

namespace entity::ecs {

// Thread-safe generator of sequential, unique type IDs.
// Maps component types to contiguous array indices without RTTI.
class Family {
 public:
  // Returns the unique sequential ID for unqualified component type `T`.
  template <typename T>
  [[nodiscard]] static auto id() noexcept -> std::size_t {
    static_assert(
        std::is_same_v<T, std::remove_cvref_t<T>>,
        "Family::id requires an unqualified component type."
    );

    static const std::size_t value = counter++;
    return value;
  }

 private:
  inline static std::atomic_size_t counter = 0;
};

// Manages the lifetime and type-erased access for a single `Storage<T>` instance.
class StorageCell {
 public:
  // --- Unchecked Accessors ---

  // Returns the typed storage, lazily creating it on first use.
  // Precondition: If already initialized, the stored type must match `T`.
  template <typename T>
  [[nodiscard]] auto get() -> Storage<T>& {
    ensureInitialized<T>();
    assumeCorrectType<T>();
    return static_cast<Storage<T>&>(*m_storage);
  }

  // Returns a const reference to the typed storage.
  // Precondition: The cell must be initialized with type `T`.
  template <typename T>
  [[nodiscard]] auto get() const noexcept -> const Storage<T>& {
    assert(m_storage && "Storage not initialized before const get.");
    assumeCorrectType<T>();
    return static_cast<const Storage<T>&>(*m_storage);
  }

  // --- Safe Accessors ---

  // Returns a pointer to the typed storage if initialized with `T`, or `nullptr`.
  template <typename T>
  [[nodiscard]] auto tryGet() noexcept -> Storage<T>* {
    if (m_storage == nullptr || !isCorrectType<T>()) {
      return nullptr;
    }
    return static_cast<Storage<T>*>(m_storage.get());
  }

  // Returns a const pointer to the typed storage if initialized with `T`, or `nullptr`.
  template <typename T>
  [[nodiscard]] auto tryGet() const noexcept -> const Storage<T>* {
    if (m_storage == nullptr || !isCorrectType<T>()) {
      return nullptr;
    }
    return static_cast<const Storage<T>*>(m_storage.get());
  }

  // --- Notifiers ---

  // Removes any component associated with `entity` from the underlying storage.
  void entityDestroyed(Entity entity) noexcept {
    if (m_storage) {
      m_storage->erase(entity);
    }
  }

 private:
  template <typename T>
  void ensureInitialized() {
    if (!m_storage) {
      m_type_id = OptionalIndex<std::size_t>(Family::id<T>());
      m_storage = std::make_unique<Storage<T>>();
    }
  }

  template <typename T>
  [[nodiscard]] auto isCorrectType() const noexcept -> bool {
    return m_type_id.hasValue() && m_type_id.value() == Family::id<T>();
  }

  template <typename T>
  void assumeCorrectType() const noexcept {
    assert(
        isCorrectType<T>() &&
        "Requested type is different than the one stored in the cell."
    );
  }

  OptionalIndex<std::size_t> m_type_id;
  std::unique_ptr<IStorage> m_storage = nullptr;
};

// Owns and manages component storages for all component types in the ECS.
class ComponentPool {
 public:
  // --- Constructors ---

  constexpr ComponentPool() noexcept = default;

  // --- Modifiers ---

  // Constructs a component of type `T` for `entity` if absent, or returns the existing one.
  template <typename T, typename... Args>
  auto emplace(Entity entity, Args&&... args) -> T& {
    return ensureStorage<T>().emplace(entity, std::forward<Args>(args)...);
  }

  // Removes the component of type `T` from `entity`.
  // Returns true if removed.
  template <typename T>
  auto erase(Entity entity) noexcept -> bool {
    if (auto* storage = tryGetStorage<T>()) {
      return storage->erase(entity);
    }
    return false;
  }

  // Clears and destroys all component storages in the pool.
  void clear() noexcept {
    m_cells.clear();
  }

  // --- Lookup ---

  // Returns a reference to component `T` of `entity`.
  // Precondition: `has<T>(entity)` is true.
  template <typename T>
  [[nodiscard]] auto get(Entity entity) noexcept -> T& {
    return getStorage<T>().get(entity);
  }

  // Returns a const reference to component `T` of `entity`.
  // Precondition: `has<T>(entity)` is true.
  template <typename T>
  [[nodiscard]] auto get(Entity entity) const noexcept -> const T& {
    return getStorage<T>().get(entity);
  }

  // Returns a pointer to component `T` of `entity`, or `nullptr` if not found.
  template <typename T>
  [[nodiscard]] auto tryGet(Entity entity) noexcept -> T* {
    if (auto* storage = tryGetStorage<T>()) {
      return storage->tryGet(entity);
    }
    return nullptr;
  }

  // Returns a const pointer to component `T` of `entity`, or `nullptr` if not found.
  template <typename T>
  [[nodiscard]] auto tryGet(Entity entity) const noexcept -> const T* {
    if (const auto* storage = tryGetStorage<T>()) {
      return storage->tryGet(entity);
    }
    return nullptr;
  }

  // Returns true if `entity` has a component of type `T`.
  template <typename T>
  [[nodiscard]] auto has(Entity entity) const noexcept -> bool {
    if (const auto* storage = tryGetStorage<T>()) {
      return storage->contains(entity);
    }
    return false;
  }

  // Returns a read-only view over all entities that have component `T`.
  template <typename T>
  [[nodiscard]] auto view() const noexcept -> View<T> {
    return View(tryGetStorage<T>());
  }

  // --- Notifiers ---

  // Removes all components associated with `entity` across all storages.
  void entityDestroyed(Entity entity) noexcept {
    for (auto& cell : m_cells) {
      cell.entityDestroyed(entity);
    }
  }

 private:
  // --- Storage Access ---

  template <typename T>
  [[nodiscard]] auto ensureStorage() -> Storage<T>& {
    return ensureCell<T>().template get<T>();
  }

  // Precondition: Storage for `T` must be initialized.
  template <typename T>
  [[nodiscard]] auto getStorage() noexcept -> Storage<T>& {
    auto* cell = tryGetCell<T>();
    assert(cell && "Storage not initialized before get.");
    return cell->template get<T>();
  }

  // Precondition: Storage for `T` must be initialized.
  template <typename T>
  [[nodiscard]] auto getStorage() const noexcept -> const Storage<T>& {
    const auto* cell = tryGetCell<T>();
    assert(cell && "Storage not initialized before const get.");
    return cell->template get<T>();
  }

  template <typename T>
  [[nodiscard]] auto tryGetStorage() noexcept -> Storage<T>* {
    if (auto* cell = tryGetCell<T>()) {
      return cell->template tryGet<T>();
    }
    return nullptr;
  }

  template <typename T>
  [[nodiscard]] auto tryGetStorage() const noexcept -> const Storage<T>* {
    if (const auto* cell = tryGetCell<T>()) {
      return cell->template tryGet<T>();
    }
    return nullptr;
  }

  // --- Cell Access ---

  template <typename T>
  [[nodiscard]] auto ensureCell() -> StorageCell& {
    const auto type_id = Family::id<T>();
    if (type_id >= m_cells.size()) {
      m_cells.resize(type_id + 1);
    }
    return m_cells[type_id];
  }

  template <typename T>
  [[nodiscard]] auto tryGetCell() noexcept -> StorageCell* {
    const auto type_id = Family::id<T>();
    if (type_id >= m_cells.size()) {
      return nullptr;
    }
    return &m_cells[type_id];
  }

  template <typename T>
  [[nodiscard]] auto tryGetCell() const noexcept -> const StorageCell* {
    const auto type_id = Family::id<T>();
    if (type_id >= m_cells.size()) {
      return nullptr;
    }
    return &m_cells[type_id];
  }

  // A vector of cells, indexed by the `Family::id`.
  std::vector<StorageCell> m_cells;
};

}  // namespace entity::ecs

#endif  // ENTITY_ECS_COMPONENT_POOL_HPP
