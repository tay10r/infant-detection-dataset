#include "generator.h"

#include "obj_model.h"
#include "obj_parser.h"
#include "renderer.h"
#include "scene.h"
#include "stl.h"

#include <random>

namespace {

using vec3 = bvh::v2::Vec<float, 3>;

constexpr auto pi{ 3.1415926535897932F };

auto
to_radians(const float degrees) -> float
{
  return (degrees / 180.0F) * pi;
}

struct model_stats final
{
  vec3 avg;

  vec3 max;

  vec3 min;
};

class generator_impl final : public generator
{
public:
  explicit generator_impl(int seed)
    : rng_(seed)
  {
  }

  void load_nursery(const char* path) override { nursery_ = parse_obj_file(path); }

  void load_baby_state(const char* obj_path) override
  {
    auto m = parse_obj_file(obj_path);
    const auto s = compute_stats(m.vertices);
    m.vertices = translate(vec3(-s.avg[0], -s.avg[1], -s.max[2]), m.vertices);
    baby_states_.emplace_back(std::move(m));
  }

  void load_floor_texture(const char* path) override
  {
    auto t = std::make_shared<texture>();
    t->load(path);
    floor_textures_.emplace_back(std::move(t));
  }

  void load_blanket_texture(const char* path) override
  {
    auto t = std::make_shared<texture>();
    t->load(path);
    blanket_textures_.emplace_back(std::move(t));
  }

  void load_painting_texture(const char* path) override
  {
    auto t = std::make_shared<texture>();
    t->load(path);
    painting_textures_.emplace_back(std::move(t));
  }

  auto generate_scene() -> scene override
  {
    // for cabinet, crib, chair
    const std::array<int, 4> p1{ 0xad8e72, 0xa77946, 0xdcd5c9, 0x2d2016 };
    // for baby skin color
    const std::array<int, 5> p2{ 0xe0ccc8, 0xe9b698, 0xc99d87, 0x996152, 0x39272d };
    // for baby clothing color
    const std::array<int, 5> p3{ 0xffffff, 0x1288d7, 0x050203, 0x2f7c5f, 0xebb54c };
    // for baby iris color
    const std::array<int, 3> p4{ 0x7182a0, 0x662b0b, 0x45635b };

    auto& baby_state = sample_obj(baby_states_);
    const auto baby_spawn_point = sample_stl(baby_spawn_area_);
    auto baby_vertices = std::move(baby_state.vertices);
    baby_state.vertices = translate(baby_spawn_point, baby_vertices);

    std::uniform_int_distribution<int> boolean_dist(0, 1);

    scene scn;
    scn.floor_texture = sample_texture(floor_textures_);
    scn.blanket_texture = sample_texture(blanket_textures_);
    scn.painting_texture = sample_texture(painting_textures_);
    scn.cabinet_color = sample_color(p1);
    scn.chair_color = sample_color(p1);
    scn.crib_color = sample_color(p1);
    scn.baby_skin_color = sample_color(p2);
    scn.baby_clothes_color = sample_color(p3);
    scn.baby_iris_color = sample_color(p4);
    scn.insert(nursery_);
    scn.insert(baby_state);
    scn.floor_direction = boolean_dist(rng_) ? true : false;
    scn.blanket_direction = boolean_dist(rng_) ? true : false;
    scn.commit();

    baby_state.vertices = std::move(baby_vertices);

    return scn;
  }

  auto generate_camera() -> camera override
  {
    std::uniform_real_distribution<float> gamma_dist(1.8F, 2.4F);
    std::uniform_real_distribution<float> fov_dist(60.0F, 120.0F);

    camera cam;
    cam.gamma = gamma_dist(rng_);
    cam.pos = sample_stl(camera_spawn_area_);
    cam.fov = to_radians(fov_dist(rng_));
    // look in the general direction of where the baby is spawning.
    cam.look_at(sample_stl(baby_spawn_area_));
    return cam;
  }

  auto generate_light() -> light override
  {
    std::uniform_real_distribution<float> radius_dist(0.01F, 0.1F);
    std::uniform_real_distribution<float> light_dist(0.05F, 1.0F);
    light l;
    l.position = sample_stl(light_spawn_area_);
    l.emission = light_dist(rng_);
    l.radius = radius_dist(rng_);
    return l;
  }

  void load_light_spawn_area(const char* stl_path) override { light_spawn_area_.load(stl_path); }

  void load_baby_spawn_area(const char* stl_path) override { baby_spawn_area_.load(stl_path); }

  void load_camera_spawn_area(const char* stl_path) override { camera_spawn_area_.load(stl_path); }

  auto generate_seed() -> int override
  {
    std::uniform_int_distribution<int> dist(0, 1'000'000'000);
    return dist(rng_);
  }

protected:
  auto sample_obj(std::vector<obj_model>& models) -> obj_model&
  {
    std::uniform_int_distribution<size_t> dist(0, models.size() - 1);
    return models.at(dist(rng_));
  }

  static auto translate(const vec3& xyz, const std::vector<std::array<float, 3>>& coords)
    -> std::vector<std::array<float, 3>>
  {
    std::vector<std::array<float, 3>> result(coords.size());
    for (size_t i = 0; i < coords.size(); i++) {
      const auto& c = coords[i];
      vec3 tmp(c[0], c[1], c[2]);
      tmp = xyz + tmp;
      result[i] = std::array<float, 3>{ tmp[0], tmp[1], tmp[2] };
    }
    return result;
  }

  static auto compute_stats(const std::vector<std::array<float, 3>>& coords) -> model_stats
  {
    vec3 sum{ 0, 0, 0 };
    vec3 max{ 0, 0, 0 };
    vec3 min{ 0, 0, 0 };

    if (coords.size() > 0) {
      max = vec3(coords[0][0], coords[0][1], coords[0][2]);
      min = max;
    }

    for (auto& v : coords) {
      vec3 tmp(v[0], v[1], v[2]);
      sum = sum + tmp;
      max = bvh::v2::robust_max(max, tmp);
      min = bvh::v2::robust_min(min, tmp);
    }

    const auto avg = sum * (1.0F / static_cast<float>(coords.size()));

    return model_stats{ avg, max, min };
  }

  template<size_t N>
  [[nodiscard]] auto sample_color(const std::array<int, N>& colors) -> vec3
  {
    std::uniform_int_distribution<int> dist(0, N - 1);
    const auto c = colors.at(dist(rng_));
    return vec3(static_cast<float>((c & 0xff0000) >> 16) / 255.0F,
                static_cast<float>((c & 0x00ff00) >> 8) / 255.0F,
                static_cast<float>(c & 0x0000ff) / 255.0F);
  }

  auto sample_texture(std::vector<std::shared_ptr<texture>>& textures) -> std::shared_ptr<texture>
  {
    std::uniform_int_distribution<size_t> dist(0, textures.size() - 1);
    const auto index = dist(rng_);
    return textures.at(index);
  }

  auto sample_stl(stl_model& m) -> vec3
  {
    auto to_vec3 = [](const std::array<float, 3>& v) -> vec3 { return vec3(v[0], v[1], v[2]); };

    while (true) {

      std::uniform_int_distribution<size_t> dist(0, m.tris.size() - 1);

      const auto& tri = m.tris.at(dist(rng_));

      const auto p0 = to_vec3(tri.p[0]);
      const auto p1 = to_vec3(tri.p[1]);
      const auto p2 = to_vec3(tri.p[2]);
      const auto e0 = p1 - p0;
      const auto e1 = p2 - p0;

      std::uniform_real_distribution<float> edge_dist(0, 1.0F);

      // NOTE : This distribution of points on the triangle is going to be
      //        skewed when the triangle does not have equally sized edges.
      //        Ensure that the spawn areas contain triangles that have
      //        equal edge lengths.
      const auto u = edge_dist(rng_);
      const auto v = edge_dist(rng_);
      if ((u + v) <= 1.0F) {
        return p0 + u * e0 + v * e1;
      }
    }
  }

private:
  std::mt19937 rng_;

  obj_model nursery_;

  std::vector<obj_model> baby_states_;

  std::vector<std::shared_ptr<texture>> floor_textures_;

  std::vector<std::shared_ptr<texture>> blanket_textures_;

  std::vector<std::shared_ptr<texture>> painting_textures_;

  stl_model light_spawn_area_;

  stl_model baby_spawn_area_;

  stl_model camera_spawn_area_;
};

} // namespace

auto
generator::create(int seed) -> std::unique_ptr<generator>
{
  return std::make_unique<generator_impl>(seed);
}
