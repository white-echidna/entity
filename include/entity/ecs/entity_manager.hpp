#pragma once

#ifndef ENTITY_ECS_ENTITY_MANAGER_HPP
#define ENTITY_ECS_ENTITY_MANAGER_HPP

#include <entity/export.hpp>

#include <entity/ecs/entity.hpp>

#include <memory>

namespace entity::ecs {

// Manages the lifecycle, versioning, and ID recycling of entities.
class ENTITY_EXPORT EntityManager {
 public:
  // --- Constructors ---

  EntityManager();

  // --- Rule of Five ---

  ~EntityManager();

  EntityManager(const EntityManager&) = delete;
  auto operator=(const EntityManager&) -> EntityManager& = delete;

  EntityManager(EntityManager&&) noexcept;
  auto operator=(EntityManager&&) noexcept -> EntityManager&;

  // --- Modifiers ---

  // Creates and returns a new unique entity.
  [[nodiscard]] auto create() -> Entity;

  // Destroys `entity` and invalidates it. Does nothing if `entity` is already invalid.
  void destroy(Entity entity);

  // --- Lookup ---

  // Returns true if `entity` is currently alive and its version matches the active slot.
  [[nodiscard]] auto isValid(Entity entity) const noexcept -> bool;

 private:
  struct Impl;
  std::unique_ptr<Impl> m_pimpl;
};

}  // namespace entity::ecs

#endif  // ENTITY_ECS_ENTITY_MANAGER_HPP
