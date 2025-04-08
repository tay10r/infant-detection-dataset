#include <cradle/obj_model.h>

#include <array>
#include <fstream>
#include <vector>

namespace cradle {

namespace {

using float2 = bvh::v2::Vec<float, 2>;

using float3 = bvh::v2::Vec<float, 3>;

struct node
{
  virtual ~node() = default;

  virtual void emit(std::ostream&) const = 0;
};

struct face_node final : public node
{
  using int3 = std::array<int, 3>;

  int3 indices[3];

  void emit(std::ostream& stream) const override
  {
    stream << 'f';
    for (auto& i : indices) {
      stream << ' ';
      stream << (i[0] + 1) << '/' << (i[1] + 1) << '/' << (i[2] + 1);
    }
    stream << '\n';
  }
};

class obj_builder_impl;

class obj_model_impl final : public obj_model
{
public:
  friend obj_builder_impl;

  void save(const char* path) const override
  {
    std::ofstream file(path);

    for (auto& v : vertices_) {
      file << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';
    }

    for (auto& vn : normals_) {
      file << "vn " << vn[0] << ' ' << vn[1] << ' ' << vn[2] << '\n';
    }

    for (auto& vt : texcoords_) {
      file << "vt " << vt[0] << ' ' << vt[1] << '\n';
    }

    for (auto& n : nodes_) {
      n->emit(file);
    }
  }

private:
  std::vector<float3> vertices_;

  std::vector<float3> normals_;

  std::vector<float2> texcoords_;

  std::vector<std::unique_ptr<node>> nodes_;
};

class obj_builder_impl final : public obj_builder
{
public:
  explicit obj_builder_impl(const int seed)
    : rng_(seed)
  {
  }

  auto add_vertex(const vec3& pos, const vec3& normal, const vec2& texcoord) -> size_t override
  {
    // TODO : only store unique attributes and map the ID correctly for the OBJ model.
    model_.vertices_.emplace_back(pos);
    model_.normals_.emplace_back(normal);
    model_.texcoords_.emplace_back(texcoord);
    return model_.vertices_.size() - 1;
  }

  void add_face(const size_t a, const size_t b, const size_t c) override
  {
    const size_t tmp[3]{ a, b, c };

    face_node n;

    for (size_t i = 0; i < 3; i++) {
      n.indices[i][0] = tmp[i];
      n.indices[i][1] = tmp[i];
      n.indices[i][2] = tmp[i];
    }

    model_.nodes_.emplace_back(std::make_unique<face_node>(n));
  }

  auto build() -> std::unique_ptr<obj_model> override { return std::make_unique<obj_model_impl>(std::move(model_)); }

  auto rng() -> rng_type& override { return rng_; }

  void add_spawn_plane(const vec3& center,
                       const vec3& normal,
                       const vec3& tangent,
                       const vec2& size,
                       spawn_area_kind) override
  {
    // TODO
  }

  void sample_spawn_point(spawn_area_kind kind) override
  {
    // TODO
  }

private:
  std::mt19937 rng_;

  obj_model_impl model_;
};

} // namespace

auto
obj_builder::create(const int seed) -> std::unique_ptr<obj_builder>
{
  return std::make_unique<obj_builder_impl>(seed);
}

void
obj_builder::add_plane(const vec3& center, const vec3& normal, const vec3& tangent, const vec2& size)
{
  const float x_deltas[4]{ 1, 1, -1, -1 };
  const float y_deltas[4]{ 1, -1, -1, 1 };

  const auto bitangent = bvh::v2::cross(normal, tangent);

  size_t indices[4]{ 0, 0, 0, 0 };

  for (auto i = 0; i < 4; i++) {

    const auto p = center + tangent * x_deltas[i] + bitangent * y_deltas[i];

    const auto u = x_deltas[i] * 0.5F + 0.5F;
    const auto v = y_deltas[i] * 0.5F + 0.5F;

    indices[i] = add_vertex(p, normal, vec2(u, v));
  }

  add_face(indices[0], indices[1], indices[2]);
  add_face(indices[2], indices[3], indices[0]);
}

} // namespace cradle
