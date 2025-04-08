#include "generator.h"
#include "path_tracer.h"
#include "scene.h"

#include <cradle/generator.h>
#include <cradle/obj_model.h>

#include <stb_image_write.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <cstdlib>

#define MODEL_DIR PROJECT_SOURCE_DIR "models/"
#define TEXTURE_DIR PROJECT_SOURCE_DIR "textures/"
#define SPAWN_DIR PROJECT_SOURCE_DIR "spawn/"

namespace {

namespace fs = std::filesystem;

void
generate_samples(const fs::path& out_dir, generator& gen, const int num_samples)
{
  std::filesystem::create_directory(out_dir);

  for (auto i = 0; i < num_samples; i++) {
    const auto scn = gen.generate_scene();
    const auto cam = gen.generate_camera();
    const auto l = gen.generate_light();

    { // input
      const auto data = render_color(scn, cam, l, gen.generate_seed());
      std::ostringstream name_stream;
      name_stream << std::setw(4) << std::setfill('0') << i << ".png";
      const auto path = (out_dir / name_stream.str()).string();
      stbi_write_png(path.c_str(), cam.width, cam.height, 3, data.data(), cam.width * 3);
    }

    { // label
      const auto data = render_mask(scn, cam);
      std::ostringstream name_stream;
      name_stream << std::setw(4) << std::setfill('0') << i << "_mask.png";
      const auto path = (out_dir / name_stream.str()).string();
      stbi_write_png(path.c_str(), cam.width, cam.height, 3, data.data(), cam.width * 3);
    }

    std::cout << "[" << i << "/" << num_samples << "]" << std::endl;
  }
}

} // namespace

auto
main() -> int
{
  // override old behavior if this file exists.
  if (fs::exists(fs::path("config.json"))) {
    std::ifstream file("config.json");
    const auto root = nlohmann::json::parse(file);
    auto gen = cradle::generator::create(root);
    auto builder = cradle::obj_builder::create();
    gen->generate(*builder);
    auto obj = builder->build();
    obj->save("result.obj");
    return EXIT_SUCCESS;
  }

  auto gen = generator::create(/*seed=*/0);
  gen->load_nursery(MODEL_DIR "nursery.obj");
  gen->load_baby_state(MODEL_DIR "baby_sleeping.obj");
  gen->load_baby_state(MODEL_DIR "baby_sleeping_side.obj");
  gen->load_baby_state(MODEL_DIR "baby_sleeping_belly.obj");
  gen->load_baby_state(MODEL_DIR "baby_sitting.obj");
  gen->load_baby_state(MODEL_DIR "baby_crawling.obj");
  gen->load_baby_state(MODEL_DIR "baby_standing.obj");
  gen->load_baby_state(MODEL_DIR "baby_standing_arms_up.obj");
  gen->load_floor_texture(TEXTURE_DIR "Carpet001/Carpet001_1K-PNG_Color.png");
  gen->load_floor_texture(TEXTURE_DIR "Wood013/Wood013_1K-PNG_Color.png");
  gen->load_floor_texture(TEXTURE_DIR "Wood092/Wood092_1K-PNG_Color.png");
  gen->load_floor_texture(TEXTURE_DIR "WoodFloor028/WoodFloor028_1K-PNG_Color.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_1.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_2.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_3.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_4.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_5.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_6.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_7.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_8.png");
  gen->load_blanket_texture(TEXTURE_DIR "blankets/blanket_9.png");
  gen->load_painting_texture(TEXTURE_DIR "paintings/painting_1.png");
  gen->load_painting_texture(TEXTURE_DIR "paintings/painting_2.png");
  gen->load_painting_texture(TEXTURE_DIR "paintings/painting_3.png");
  gen->load_painting_texture(TEXTURE_DIR "paintings/painting_4.png");
  gen->load_painting_texture(TEXTURE_DIR "paintings/painting_5.png");
  gen->load_light_spawn_area(SPAWN_DIR "light.stl");
  gen->load_baby_spawn_area(SPAWN_DIR "baby.stl");
  gen->load_camera_spawn_area(SPAWN_DIR "camera.stl");

  generate_samples("train", *gen, 10000);

  return EXIT_SUCCESS;
}
