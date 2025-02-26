#include "scene.h"

#include "exceptions.h"
#include "obj_model.h"

#include <bvh/v2/default_builder.h>
#include <bvh/v2/executor.h>
#include <bvh/v2/thread_pool.h>

#include <stb_image.h>

#include <sstream>

void
texture::load(const char* path)
{
  int w{};
  int h{};
  auto* pixels = stbi_load(path, &w, &h, nullptr, 3);
  if (!pixels) {
    std::ostringstream error;
    error << "Failed to open \"" << path << "\".";
    throw texture_error(error.str());
  }

  width = w;
  height = h;
  data.resize(w * h * 3);

  const auto num_pixels = w * h;

  for (auto i = 0; i < num_pixels; i++) {
    auto* dst = data.data() + i * 3;
    const auto* src = pixels + i * 3;
    dst[0] = static_cast<float>(src[0]) / 255.0F;
    dst[1] = static_cast<float>(src[1]) / 255.0F;
    dst[2] = static_cast<float>(src[2]) / 255.0F;
  }

  stbi_image_free(pixels);
}

void
scene::insert(const obj_model& m)
{
  using vec2 = bvh::v2::Vec<float, 2>;
  using vec3 = bvh::v2::Vec<float, 3>;

  auto to_vec2 = [](const std::array<float, 2>& xy) -> vec2 { return vec2(xy[0], xy[1]); };
  auto to_vec3 = [](const std::array<float, 3>& xyz) -> vec3 { return vec3(xyz[0], xyz[1], xyz[2]); };

  for (const auto& f : m.faces) {
    {
      const auto a = to_vec3(m.vertices.at(f.position.at(0)));
      const auto b = to_vec3(m.vertices.at(f.position.at(1)));
      const auto c = to_vec3(m.vertices.at(f.position.at(2)));
      const bvh::v2::Tri<float, 3> tri(a, b, c);
      primitives.emplace_back(tri);
    }

    {
      const auto a = to_vec2(m.texcoords.at(f.texcoord.at(0)));
      const auto b = to_vec2(m.texcoords.at(f.texcoord.at(1)));
      const auto c = to_vec2(m.texcoords.at(f.texcoord.at(2)));
      texcoords.emplace_back(a);
      texcoords.emplace_back(b);
      texcoords.emplace_back(c);
    }

    {
      const auto a = to_vec3(m.normals.at(f.normal.at(0)));
      const auto b = to_vec3(m.normals.at(f.normal.at(1)));
      const auto c = to_vec3(m.normals.at(f.normal.at(2)));
      normals.emplace_back(a);
      normals.emplace_back(b);
      normals.emplace_back(c);
    }
  }

  for (auto& fc : m.face_classes) {
    face_classes.emplace_back(fc);
  }
}

void
scene::commit()
{
  using vec2 = bvh::v2::Vec<float, 2>;
  using vec3 = bvh::v2::Vec<float, 3>;
  using bbox3 = bvh::v2::BBox<float, 3>;

  bvh::v2::ThreadPool thread_pool;
  bvh::v2::ParallelExecutor executor(thread_pool);

  const auto& tris = primitives;

  // Get triangle centers and bounding boxes (required for BVH builder)
  std::vector<bbox3> bboxes(tris.size());
  std::vector<vec3> centers(tris.size());
  executor.for_each(0, tris.size(), [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      bboxes[i] = tris[i].get_bbox();
      centers[i] = tris[i].get_center();
    }
  });

  typename bvh::v2::DefaultBuilder<node>::Config config;

  config.quality = bvh::v2::DefaultBuilder<node>::Quality::High;

  scene_bvh = bvh::v2::DefaultBuilder<node>::build(thread_pool, bboxes, centers, config);

  std::vector<tri> tmp_tris(tris.size());
  std::vector<vec2> tmp_texcoords(tris.size() * 3);
  std::vector<vec3> tmp_normals(tris.size() * 3);
  std::vector<obj_class> tmp_classes(tris.size());
  executor.for_each(0, tris.size(), [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      auto j = scene_bvh.prim_ids[i];
      tmp_tris[i] = tris[j];
      tmp_texcoords[i * 3 + 0] = texcoords[j * 3 + 0];
      tmp_texcoords[i * 3 + 1] = texcoords[j * 3 + 1];
      tmp_texcoords[i * 3 + 2] = texcoords[j * 3 + 2];
      tmp_normals[i * 3 + 0] = normals[j * 3 + 0];
      tmp_normals[i * 3 + 1] = normals[j * 3 + 1];
      tmp_normals[i * 3 + 2] = normals[j * 3 + 2];
      tmp_classes[i] = face_classes[j];
    }
  });
  primitives = std::move(tmp_tris);
  texcoords = std::move(tmp_texcoords);
  normals = std::move(tmp_normals);
  face_classes = std::move(tmp_classes);
}
