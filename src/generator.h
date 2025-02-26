#pragma once

#include <memory>

struct scene;
struct camera;
struct light;

class generator
{
public:
  static auto create(int seed) -> std::unique_ptr<generator>;

  virtual ~generator() = default;

  virtual void load_nursery(const char* obj_path) = 0;

  virtual void load_baby_state(const char* obj_path) = 0;

  virtual void load_floor_texture(const char* texture_path) = 0;

  virtual void load_blanket_texture(const char* texture_path) = 0;

  virtual void load_painting_texture(const char* texture_path) = 0;

  virtual void load_light_spawn_area(const char* stl_path) = 0;

  virtual void load_baby_spawn_area(const char* stl_path) = 0;

  virtual void load_camera_spawn_area(const char* stl_path) = 0;

  virtual auto generate_scene() -> scene = 0;

  virtual auto generate_camera() -> camera = 0;

  virtual auto generate_light() -> light = 0;

  virtual auto generate_seed() -> int = 0;
};
