#include <gtest/gtest.h>

#include <cradle/exceptions.h>
#include <cradle/renderer.h>

TEST(renderer, throw_invalid_renderer)
{
  EXPECT_THROW(cradle::renderer::create(static_cast<cradle::renderer::mode>(-2)), cradle::invalid_argument);
}
