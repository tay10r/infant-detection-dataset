#pragma once

#include <cradle/generator.h>

namespace cradle {

class cabinet_generator final : public generator
{
public:
  cabinet_generator(const nlohmann::json& config);

  auto generate(obj_builder& builder) -> bool override;
};

} // namespace cradle
