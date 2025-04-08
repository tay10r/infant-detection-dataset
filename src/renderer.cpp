#include <cradle/renderer.h>

#include <cradle/exceptions.h>

namespace cradle {

namespace {

class opengl_renderer final : public renderer
{
public:
};

} // namespace

auto
renderer::create(const mode m) -> std::unique_ptr<renderer>
{
  switch (m) {
    case mode::visible:
      break;
    case mode::near_ir:
      break;
    case mode::mask:
      break;
  }

  throw invalid_argument("bad renderer mode");
}

} // namespace cradle
