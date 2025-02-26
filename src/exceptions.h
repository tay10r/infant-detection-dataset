#pragma once

#include <stdexcept>

class obj_error final : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

class stl_error final : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};

class texture_error final : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};
