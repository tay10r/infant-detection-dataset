#pragma once

#include <nlohmann/json_fwd.hpp>

#include <bvh/v2/vec.h>

#include <memory>

namespace cradle {

class obj_builder;

class generator
{
public:
  using vec2 = bvh::v2::Vec<float, 2>;

  using vec3 = bvh::v2::Vec<float, 3>;

  static auto create(const nlohmann::json& config) -> std::unique_ptr<generator>;

  virtual ~generator() = default;

  [[nodiscard]] virtual auto generate(obj_builder& builder) -> bool = 0;
};

} // namespace cradle
