#include <entity/ecs/entity_manager.hpp>

#include <entity/ecs/entity.hpp>

#include <exception>
#include <memory>
#include <queue>
#include <vector>

#include <cassert>
#include <cstdint>

namespace entity::ecs {

namespace {

// Manages a pool of reusable entity IDs.
class Freelist {
 public:
  // Returns a recycled ID from the queue, or allocates a new sequential ID.
  [[nodiscard]] auto obtain() -> EntityType {
    if (m_free_ids.empty()) {
      if (m_next_id > Entity::kIdMax) [[unlikely]] {
        // All entity ID slots have been exhausted.
        // Terminate to prevent undefined behavior.
        std::terminate();
      }

      // Allocate a brand new sequential ID.
      return m_next_id++;
    }

    // Reuse the oldest released ID from the queue.
    const auto id = m_free_ids.front();
    m_free_ids.pop();

    return id;
  }

  // Pushes `id` to the back of the free queue for future reuse.
  void release(EntityType id) {
    m_free_ids.push(id);
  }

 private:
  std::queue<EntityType> m_free_ids;
  EntityType m_next_id{};
};

// Tracks the current version and liveness state for each entity ID slot.
class SlotTable {
 public:
  // Marks the slot at `id` as alive and returns its current version.
  [[nodiscard]] auto activate(EntityType id) -> EntityType {
    if (id >= m_slots.size()) {
      m_slots.resize(id + 1);
    }
    assert(!m_slots[id].alive && "activate called on an already-alive slot.");
    m_slots[id].alive = true;
    return m_slots[id].version;
  }

  // Marks the slot at `id` as dead and increments its version.
  // Returns false if the slot has exhausted all versions and must be retired.
  [[nodiscard]] auto deactivate(EntityType id) noexcept -> bool {
    assert(id < m_slots.size() && "deactivate called on never-activated slot.");
    assert(m_slots[id].alive && "deactivate called on already-dead slot.");

    m_slots[id].alive = false;
    if (m_slots[id].version == Entity::kVersionMax) {
      // All generations for this ID are exhausted.
      // Retire the slot permanently to prevent ABA collisions.
      return false;
    }

    ++m_slots[id].version;
    return true;
  }

  // Returns true if an `entity` is currently valid.
  [[nodiscard]] auto isValid(Entity entity) const noexcept -> bool {
    const auto id = entity.id();

    if (id >= m_slots.size()) {
      return false;  // Slot has never been allocated.
    }

    const auto& slot = m_slots[id];
    // Entity is valid if it's alive and has the correct version.
    return slot.alive && (entity.version() == slot.version);
  }

 private:
  struct Slot {
    std::uint16_t version = 0;
    bool alive = false;
  };
  // Slots are aligned to 4 bytes for fast access and cache locality.
  static_assert(sizeof(Slot) == 4);

  std::vector<Slot> m_slots;
};

}  // namespace

struct EntityManager::Impl {
  Freelist freelist;
  SlotTable slot_table;
};

// --- Constructors ---

EntityManager::EntityManager()
    : m_pimpl(std::make_unique<Impl>()) {}

// --- Rule of Five ---

EntityManager::~EntityManager() = default;

EntityManager::EntityManager(EntityManager&&) noexcept = default;
auto EntityManager::operator=(EntityManager&&) noexcept
    -> EntityManager& = default;

// --- Modifiers ---

auto EntityManager::create() -> Entity {
  const auto id = m_pimpl->freelist.obtain();
  const auto version = m_pimpl->slot_table.activate(id);
  return {id, version};
}

void EntityManager::destroy(Entity entity) {
  if (!isValid(entity)) {
    return;
  }
  // Invalidate the current entity and recycle its ID only if versions remain.
  if (m_pimpl->slot_table.deactivate(entity.id())) {
    m_pimpl->freelist.release(entity.id());
  }
}

// --- Lookup ---

auto EntityManager::isValid(Entity entity) const noexcept -> bool {
  return m_pimpl->slot_table.isValid(entity);
}

}  // namespace entity::ecs
