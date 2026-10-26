#pragma once

#ifndef ENTITY_STRONG_ALIAS_HPP
#define ENTITY_STRONG_ALIAS_HPP

#include <utility>

namespace entity {

// A generic template for creating strongly-typed aliases around `T`.
// `Tag` is an empty struct used to distinguish aliases of the same underlying type.
template <typename Tag, typename T>
class StrongAlias {
 public:
  // --- Constructors ---

  constexpr StrongAlias() noexcept = default;

  constexpr explicit StrongAlias(T v) noexcept
      : m_value(std::move(v)) {}

  // --- Accessors ---

  [[nodiscard]] constexpr auto value() noexcept -> T& {
    return m_value;
  }

  [[nodiscard]] constexpr auto value() const noexcept -> const T& {
    return m_value;
  }

 private:
  T m_value{};
};

}  // namespace entity

#endif  // ENTITY_STRONG_ALIAS_HPP
