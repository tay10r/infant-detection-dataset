#pragma once

#include <array>
#include <vector>

struct stl_tri final
{
  using float3 = std::array<float, 3>;

  float3 p[3];
};

struct stl_model final
{
  std::vector<stl_tri> tris;

  void load(const char* filename);
};
