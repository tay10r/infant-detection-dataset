#pragma once

#include <bvh/v2/vec.h>

#include <array>
#include <memory>
#include <random>

#include <stddef.h>

namespace cradle {

/// @brief Describes the surface that a spawn area belongs to.
///        Useful for determining what objects may be spawned on
///        a given area.
enum class spawn_area_kind
{
  floor,
  wall,
  ceiling
};

class obj_model
{
public:
  virtual ~obj_model() = default;

  virtual void save(const char* path) const = 0;
};

class obj_builder
{
public:
  using rng_type = std::mt19937;

  using vec2 = bvh::v2::Vec<float, 2>;

  using vec3 = bvh::v2::Vec<float, 3>;

  using int3 = std::array<int, 3>;

  static auto create(int seed = 0) -> std::unique_ptr<obj_builder>;

  virtual ~obj_builder() = default;

  [[nodiscard]] virtual auto add_vertex(const vec3& pos, const vec3& normal, const vec2& texcoord) -> size_t = 0;

  virtual void add_face(size_t a, size_t b, size_t c) = 0;

  [[nodiscard]] virtual auto build() -> std::unique_ptr<obj_model> = 0;

  [[nodiscard]] virtual auto rng() -> rng_type& = 0;

  void add_plane(const vec3& center, const vec3& normal, const vec3& tangent, const vec2& size);

  /// @brief Generates a plane on which objects may be generated on.
  virtual void add_spawn_plane(const vec3& center,
                               const vec3& normal,
                               const vec3& tangent,
                               const vec2& size,
                               spawn_area_kind kind) = 0;

  virtual void sample_spawn_point(spawn_area_kind kind) = 0;
};

} // namespace cradle
