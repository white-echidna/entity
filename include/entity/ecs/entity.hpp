#pragma once

#ifndef ENTITY_ECS_ENTITY_HPP
#define ENTITY_ECS_ENTITY_HPP

#include <entity/strong_alias.hpp>

#include <limits>

#include <cassert>
#include <cstdint>

namespace entity::ecs {

// The underlying unsigned integral type for an entity.
using EntityType = std::uint32_t;

struct EntityTag {};

// A lightweight, versioned identifier for an entity.
struct Entity : StrongAlias<EntityTag, EntityType> {
  // --- Bit Layout Constants ---

  static constexpr EntityType kBits = std::numeric_limits<EntityType>::digits;
  static constexpr EntityType kIdBits = 20;
  static constexpr EntityType kVersionBits = 12;

  static constexpr EntityType kIdMask = (EntityType{1} << kIdBits) - 1;
  static constexpr EntityType kVersionMask = ~kIdMask;

  static constexpr EntityType kMax = std::numeric_limits<EntityType>::max();
  static constexpr EntityType kIdMax = kIdMask;
  static constexpr EntityType kVersionMax = (EntityType{1} << kVersionBits) - 1;

  // --- Compile-Time Sanity Checks ---
  static_assert((kIdBits + kVersionBits) == kBits);
  static_assert((kIdMask & kVersionMask) == 0);
  static_assert((kIdMask | kVersionMask) == kMax);

  // --- Constructors ---

  using StrongAlias::StrongAlias;

  // Creates an entity by packing `id` and `version`.
  // Precondition: `id <= kIdMax` and `version <= kVersionMax`.
  constexpr Entity(EntityType id, EntityType version) noexcept
      : Entity(pack(id, version)) {}

  // Packs `id` and `version` into a raw `EntityType` value.
  // Precondition: `id <= kIdMax` and `version <= kVersionMax`.
  [[nodiscard]]
  static constexpr auto pack(EntityType id, EntityType version) noexcept
      -> EntityType {
    assert(id <= kIdMax && "Entity ID exceeds bit limit.");
    assert(version <= kVersionMax && "Entity version exceeds bit limit.");
    return ((version & kVersionMax) << kIdBits) | (id & kIdMask);
  }

  // --- Observers ---

  // Extracts the 20-bit ID part of the entity.
  [[nodiscard]] constexpr auto id() const noexcept -> EntityType {
    return value() & kIdMask;
  }

  // Extracts the 12-bit version part of the entity.
  [[nodiscard]] constexpr auto version() const noexcept -> EntityType {
    return (value() & kVersionMask) >> kIdBits;
  }
};

[[nodiscard]]
constexpr auto operator==(Entity lhs, Entity rhs) noexcept -> bool {
  return lhs.value() == rhs.value();
}

[[nodiscard]]
constexpr auto operator!=(Entity lhs, Entity rhs) noexcept -> bool {
  return !(lhs == rhs);
}

}  // namespace entity::ecs

#endif  // ENTITY_ECS_ENTITY_HPP
