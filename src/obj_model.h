#pragma once

#include "obj_class.h"

#include <array>
#include <vector>

struct face final
{
  std::array<size_t, 3> position;

  std::array<size_t, 3> texcoord;

  std::array<size_t, 3> normal;
};

struct obj_model final
{
  using float2 = std::array<float, 2>;

  using float3 = std::array<float, 3>;

  std::vector<float3> vertices;

  std::vector<float3> normals;

  std::vector<float2> texcoords;

  std::vector<face> faces;

  std::vector<obj_class> face_classes;
};
