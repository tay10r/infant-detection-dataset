#include "room_generator.h"

#include <cradle/obj_model.h>

#include <nlohmann/json.hpp>

namespace cradle {

room_generator::room_generator(const nlohmann::json& config)
  : min_size_(config.value("min_size", min_size_))
  , max_size_(config.value("max_size", max_size_))
  , min_height_(config.value("min_height", min_height_))
  , max_height_(config.value("max_height", max_height_))
{
}

auto
room_generator::generate(obj_builder& builder) -> bool
{
  auto& rng = builder.rng();

  std::uniform_real_distribution<float> size_dist(min_size_, max_size_);

  const auto x_size = size_dist(rng);
  const auto y_size = size_dist(rng);

  builder.add_plane(vec3(0, 0, 0), vec3(0, 0, -1), vec3(1, 0, 0), vec2(x_size, y_size));

  // TODO : generate walls

  return true;
}

} // namespace cradle
