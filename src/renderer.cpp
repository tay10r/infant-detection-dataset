#include "renderer.h"

#include "scene.h"

#include <bvh/v2/ray.h>

#include <algorithm>
#include <random>

#include <math.h>

namespace {

using ray = bvh::v2::Ray<float, 3>;

template<int Dim>
using vec_n = bvh::v2::Vec<float, Dim>;

using vec3 = bvh::v2::Vec<float, 3>;

class renderer final
{
public:
  renderer(const int w, const int h, const int seed)
  {
    std::mt19937 rng(seed);
    rng_.resize(w * h);
    for (auto i = 0; i < (w * h); i++) {
      rng_[i] = std::minstd_rand(rng());
    }
  }

  auto render_color(const scene& scn, const camera& cam, const light& l) -> std::vector<uint8_t>
  {
    const auto aspect = static_cast<float>(cam.width) / static_cast<float>(cam.height);
    const auto fov = std::tan(cam.fov * 0.5F);

    const auto num_pixels = cam.width * cam.height;

    const auto x_scale = 1.0F / static_cast<float>(cam.width);
    const auto y_scale = 1.0F / static_cast<float>(cam.height);

    std::vector<uint8_t> color(num_pixels * 3, 0);

    const vec3 origin(cam.pos[0], cam.pos[1], cam.pos[2]);

#pragma omp parallel for

    for (auto i = 0; i < num_pixels; i++) {

      const auto x = i % cam.width;
      const auto y = i / cam.width;

      auto& rng = rng_[i];

      std::uniform_real_distribution<float> dist(0, 1);

      vec3 sum(0, 0, 0);

      for (auto j = 0; j < cam.spp; j++) {
        const auto u = (static_cast<float>(x) + dist(rng)) * x_scale;
        const auto v = (static_cast<float>(y) + dist(rng)) * y_scale;
        const auto dx = (u * 2.0F - 1.0F) * fov * aspect;
        const auto dy = (v * 2.0F - 1.0F) * fov;
        const auto dir = bvh::v2::normalize(cam.forward + cam.right * dx + cam.up * dy);
        ray primary_ray(origin, dir);
        const auto c = trace(primary_ray, scn, l, rng, /*depth=*/0);
        sum = sum + c;
      }

      const auto avg = sum * (1.0F / static_cast<float>(cam.spp));

      const auto c = gamma_correction(tone_map(avg), cam.gamma);

      const auto r = std::clamp(static_cast<int>(c[0] * 255.0F), 0, 255);
      const auto g = std::clamp(static_cast<int>(c[1] * 255.0F), 0, 255);
      const auto b = std::clamp(static_cast<int>(c[2] * 255.0F), 0, 255);

      color[i * 3 + 0] = static_cast<uint8_t>(r);
      color[i * 3 + 1] = static_cast<uint8_t>(g);
      color[i * 3 + 2] = static_cast<uint8_t>(b);
    }

    return color;
  }

  auto render_mask(const scene& scn, const camera& cam) -> std::vector<uint8_t>
  {
    const auto aspect = static_cast<float>(cam.width) / static_cast<float>(cam.height);
    const auto fov = std::tan(cam.fov * 0.5F);

    const auto num_pixels = cam.width * cam.height;

    const auto x_scale = 1.0F / static_cast<float>(cam.width);
    const auto y_scale = 1.0F / static_cast<float>(cam.height);

    std::vector<uint8_t> color(num_pixels * 3, 0);

    const vec3 origin(cam.pos[0], cam.pos[1], cam.pos[2]);

#pragma omp parallel for

    for (auto i = 0; i < num_pixels; i++) {

      const auto x = i % cam.width;
      const auto y = i / cam.width;

      const auto u = (static_cast<float>(x) + 0.5F) * x_scale;
      const auto v = (static_cast<float>(y) + 0.5F) * y_scale;

      const auto dx = (u * 2.0F - 1.0F) * fov * aspect;
      const auto dy = (v * 2.0F - 1.0F) * fov;
      const auto dir = bvh::v2::normalize(cam.forward + cam.right * dx + cam.up * dy);

      ray primary_ray(origin, dir);

      const auto isect = scn.intersect(primary_ray);

      auto cls = obj_class::unknown;

      if (isect.has_value()) {
        cls = scn.face_classes.at(isect->primitive);
      }

      vec3 c(0, 0, 0);

      switch (cls) {
        default:
          break;
        case obj_class::baby:
          c[0] = 0.0F;
          c[1] = 0.0F;
          c[2] = 1.0F;
          break;
        case obj_class::baby_eye:
        case obj_class::baby_iris:
        case obj_class::baby_pupil:
        case obj_class::baby_head:
          c[0] = 1.0F;
          c[1] = 0.0F;
          c[2] = 1.0F;
          break;
        case obj_class::baby_hand:
          c[0] = 0.0F;
          c[1] = 1.0F;
          c[2] = 1.0F;
          break;
      }

      const auto r = std::clamp(static_cast<int>(c[0] * 255.0F), 0, 255);
      const auto g = std::clamp(static_cast<int>(c[1] * 255.0F), 0, 255);
      const auto b = std::clamp(static_cast<int>(c[2] * 255.0F), 0, 255);

      color[i * 3 + 0] = static_cast<uint8_t>(r);
      color[i * 3 + 1] = static_cast<uint8_t>(g);
      color[i * 3 + 2] = static_cast<uint8_t>(b);
    }

    return color;
  }

protected:
  [[nodiscard]] static auto gamma_correction(const float x, const float gamma) -> float
  {
    return powf(x, 1.0F / gamma);
  }

  [[nodiscard]] static auto gamma_correction(const vec3& c, const float gamma) -> vec3
  {
    return vec3(gamma_correction(c[0], gamma), gamma_correction(c[1], gamma), gamma_correction(c[2], gamma));
  }

  [[nodiscard]] static auto tone_map(const float x) -> float { return (x * (x + 0.022)) / (x * (x + 0.15) + 0.02); }

  [[nodiscard]] static auto tone_map(const vec3& c) -> vec3
  {
    return vec3(tone_map(c[0]), tone_map(c[1]), tone_map(c[2]));
  }

  [[nodiscard]] auto on_miss(const ray& r) -> vec3
  {
    const vec3 up(0, 0, -1);
    const auto level = dot(up, r.dir) * 0.5F + 0.5F;
    const vec3 lo(1.0F, 1.0F, 1.0F);
    const vec3 hi(0.5F, 0.7F, 1.0F);
    return lo + (hi - lo) * level;
  }

  template<int Dim>
  [[nodiscard]] static auto compute_attrib(const std::vector<vec_n<Dim>>& attribs,
                                           const size_t primitive,
                                           const float u,
                                           const float v) -> vec_n<Dim>
  {
    const auto a0 = attribs.at(primitive * 3 + 0);
    const auto a1 = attribs.at(primitive * 3 + 1);
    const auto a2 = attribs.at(primitive * 3 + 2);
    const auto w = 1.0F - u - v;
    const auto a = a0 * w + a1 * u + a2 * v;
    return a;
  }

  template<typename Rng>
  [[nodiscard]] static auto sample_hemisphere(const vec3& n, Rng& rng) -> vec3
  {
    vec3 v;
    std::uniform_real_distribution<float> dist(-1, 1);
    while (true) {
      v = vec3(dist(rng), dist(rng), dist(rng));
      if (dot(v, v) > 1.0F) {
        continue;
      }
      v = normalize(v);
      if (dot(v, n) < 0.0F) {
        v = -v;
      }
      break;
    }
    return v;
  }

  template<typename Rng>
  [[nodiscard]] static auto sample_xy_disc(const float radius, Rng& rng) -> vec3
  {
    vec3 v;
    std::uniform_real_distribution<float> dist(-1, 1);
    while (true) {
      v = vec3(dist(rng), dist(rng), 0.0F);
      if ((v[0] * v[0] + v[1] * v[1]) <= 1.0F) {
        break;
      }
    }
    return v;
  }

  template<typename Rng>
  [[nodiscard]] auto trace(ray& r, const scene& scn, const light& l, Rng& rng, const int depth) -> vec3
  {
    constexpr auto max_depth{ 3 };
    if (depth > max_depth) {
      return vec3(0, 0, 0);
    }

    const auto isect = scn.intersect(r);
    if (!isect) {
      return on_miss(r);
    }

    auto texcoord = compute_attrib<2>(scn.texcoords, isect->primitive, isect->u, isect->v);

    auto normal = bvh::v2::normalize(compute_attrib<3>(scn.normals, isect->primitive, isect->u, isect->v));
    // compensate for messed up normals
    if (bvh::v2::dot(normal, r.dir) > 0) {
      normal = -normal;
    }

    const auto hit_pos = r.org + r.dir * isect->distance + normal * 1.0e-3F;

    const auto albedo = compute_albedo(scn, isect->primitive, texcoord[0], texcoord[1]);

    std::uniform_int_distribution<int> bool_dist(0, 1);
    if (bool_dist(rng)) {
      const auto next_dir = sample_hemisphere(normal, rng);
      ray next_ray(hit_pos, next_dir);
      return albedo * trace(next_ray, scn, l, rng, depth + 1);
    } else {
      const auto light_delta = (l.position + sample_xy_disc(l.radius, rng)) - hit_pos;
      const auto light_dist = bvh::v2::length(light_delta);
      const auto light_dir = light_delta * (1.0F / light_dist);
      const auto emission = vec3(l.emission, l.emission, l.emission);
      const auto direct_lighting = std::max((bvh::v2::dot(normal, light_dir) * 0.5F + 0.5F), 0.0F) * emission;
      ray light_ray(hit_pos, light_dir);
      const auto light_isect = scn.intersect(light_ray);
      const auto light_isect_d =
        light_isect.has_value() ? light_isect->distance : std::numeric_limits<float>::infinity();
      const auto lighting = (light_isect_d < light_dist) ? vec3(0.0F, 0.0F, 0.0F) : direct_lighting;
      return lighting * albedo;
    }
  }

  [[nodiscard]] static auto compute_albedo(const scene& scn, const size_t primitive, const float u, const float v)
    -> vec3
  {
    const auto cls = scn.face_classes.at(primitive);
    switch (cls) {
      default:
        break;
      case obj_class::floor:
        if (scn.floor_direction) {
          return scn.floor_texture->sample(u * 10, v * 10);
        } else {
          return scn.floor_texture->sample(v * 10, u * 10);
        }
      case obj_class::mattress:
        return vec3(1, 1, 1);
      case obj_class::wall:
        return scn.wall_color;
      case obj_class::crib:
        return scn.crib_color;
      case obj_class::crib_bars:
        return scn.crib_color;
      case obj_class::blanket:
        if (scn.blanket_direction) {
          return scn.blanket_texture->sample(u * 4, v * 4);
        } else {
          return scn.blanket_texture->sample(v * 4, u * 4);
        }
      case obj_class::cabinet:
        return scn.cabinet_color;
      case obj_class::baby_hand:
      case obj_class::baby_head:
        return scn.baby_skin_color;
      case obj_class::baby:
        return scn.baby_clothes_color;
      case obj_class::baby_eye:
        return vec3(1, 1, 1);
      case obj_class::baby_iris:
        return scn.baby_iris_color;
      case obj_class::baby_pupil:
        return vec3(0, 0, 0);
      case obj_class::painting:
        return scn.painting_texture->sample(u, v);
      case obj_class::lamp_top:
        return vec3(1, 1, 1);
      case obj_class::lamp_bottom:
        return vec3(0.1F, 0.1F, 0.1F);
      case obj_class::chair:
        return scn.chair_color;
      case obj_class::toy:
        return vec3(0.5, 0.7, 1.0);
      case obj_class::name_board:
        return vec3(0.1, 0.1, 0.1);
      case obj_class::name_text:
        return vec3(1, 1, 1);
    }
    return vec3(0.8F, 0.8F, 0.8F);
  }

private:
  std::vector<std::minstd_rand> rng_;
};

} // namespace

auto
render_color(const scene& scn, const camera& cam, const light& l, int seed) -> std::vector<uint8_t>
{
  renderer r(cam.width, cam.height, seed);
  return r.render_color(scn, cam, l);
}

auto
render_mask(const scene& scn, const camera& cam) -> std::vector<uint8_t>
{
  renderer r(cam.width, cam.height, /*seed=*/0);
  return r.render_mask(scn, cam);
}
