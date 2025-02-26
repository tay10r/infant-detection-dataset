#pragma once

#include "obj_class.h"

#include <bvh/v2/bvh.h>
#include <bvh/v2/ray.h>
#include <bvh/v2/stack.h>
#include <bvh/v2/tri.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

struct obj_model;

struct intersection final
{
  size_t primitive;

  float distance;

  float u;

  float v;
};

struct texture final
{
  std::vector<float> data;

  int width{};

  int height{};

  void load(const char* path);

  auto sample(const float u, const float v) const -> bvh::v2::Vec<float, 3>
  {
    const auto x = static_cast<int>(u * width) % width;
    const auto y = static_cast<int>(v * height) % height;
    const auto* ptr = &data.at((y * width + x) * 3);
    return bvh::v2::Vec<float, 3>(ptr[0], ptr[1], ptr[2]);
  }
};

struct scene final
{
  using vec3 = bvh::v2::Vec<float, 3>;

  using tri = bvh::v2::PrecomputedTri<float>;

  using node = bvh::v2::Node<float, 3>;

  using bvh_type = bvh::v2::Bvh<node>;

  std::shared_ptr<texture> floor_texture;

  std::shared_ptr<texture> blanket_texture;

  std::shared_ptr<texture> painting_texture;

  std::vector<tri> primitives;

  std::vector<bvh::v2::Vec<float, 2>> texcoords;

  std::vector<bvh::v2::Vec<float, 3>> normals;

  std::vector<obj_class> face_classes;

  bvh_type scene_bvh;

  vec3 wall_color{ 1, 1, 1 };

  vec3 crib_color{ 1, 1, 1 };

  vec3 cabinet_color{ 1, 1, 1 };

  vec3 chair_color{ 1, 1, 1 };

  vec3 baby_skin_color{ 1, 1, 1 };

  vec3 baby_clothes_color{ 1, 1, 1 };

  vec3 baby_iris_color{ 0, 1, 0 };

  bool floor_direction{ false };

  bool blanket_direction{ false };

  void insert(const obj_model& m);

  void commit();

  [[nodiscard]] auto intersect(bvh::v2::Ray<float, 3>& r) const -> std::optional<intersection>
  {
    static constexpr size_t invalid_id = std::numeric_limits<size_t>::max();

    static constexpr size_t stack_size = 64;

    auto prim_id = invalid_id;
    auto u{ 0.0F };
    auto v{ 0.0F };

    bvh::v2::SmallStack<bvh_type::Index, stack_size> stack;

    scene_bvh.intersect<false, /*use_robust_traversal=*/true>(
      r, scene_bvh.get_root().index, stack, [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
          if (auto hit = primitives[i].intersect(r)) {
            prim_id = i;
            std::tie(r.tmax, u, v) = *hit;
          }
        }
        return prim_id != invalid_id;
      });

    if (prim_id != invalid_id) {
      return intersection{ prim_id, r.tmax, u, v };
    }

    return std::nullopt;
  }
};
