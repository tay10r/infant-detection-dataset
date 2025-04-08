#pragma once

#include <cradle/generator.h>

namespace cradle {

class room_generator final : public generator
{
public:
  room_generator(const nlohmann::json& config);

  auto generate(obj_builder& builder) -> bool override;

private:
  float min_size_{ 4 };

  float max_size_{ 5 };

  float min_height_{ 3 };

  float max_height_{ 4 };
};

} // namespace cradle
