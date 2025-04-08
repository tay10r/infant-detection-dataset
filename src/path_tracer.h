#pragma once

#include <bvh/v2/vec.h>

#include <vector>

#include <stdint.h>

struct scene;

struct light final
{
  using vec3 = bvh::v2::Vec<float, 3>;

  vec3 position{ 0, 0, -2 };

  float emission{ 0.5F };

  float radius{ 0.1F };
};

struct camera final
{
  using vec3 = bvh::v2::Vec<float, 3>;

  int width{ 1280 };

  int height{ 720 };

  float fov{ 1.0F };

  float gamma{ 2.2F };

  int spp{ 1024 };

  vec3 pos{ -3, 1, -1.5 };

  vec3 up{ 0, 0, -1 };

  vec3 right{ 0, 1, 0 };

  vec3 forward{ 1, 0, 0 };

  void look_at(const vec3& p)
  {
    const auto delta = p - pos;
    forward = bvh::v2::normalize(delta);
    const auto tmp = bvh::v2::cross(forward, vec3(0, 0, 1));
    up = bvh::v2::cross(tmp, forward);
    right = bvh::v2::cross(up, forward);
  }
};

[[nodiscard]] auto
render_color(const scene& scn, const camera& cam, const light& l, int seed) -> std::vector<uint8_t>;

[[nodiscard]] auto
render_mask(const scene& scn, const camera& cam) -> std::vector<uint8_t>;
