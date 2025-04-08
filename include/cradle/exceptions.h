#pragma once

#include <stdexcept>

namespace cradle {

using runtime_error = std::runtime_error;

class invalid_argument final : public runtime_error
{
public:
  using runtime_error::runtime_error;
};

} // namespace cradle
