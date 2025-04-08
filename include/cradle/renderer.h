#pragma once

#include <memory>

namespace cradle {

class renderer
{
public:
  enum class mode
  {
    visible,
    near_ir,
    mask,
  };

  static auto create(const mode m) -> std::unique_ptr<renderer>;

  virtual ~renderer() = default;
};

} // namespace cradle
