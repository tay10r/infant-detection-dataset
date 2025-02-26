#include "stl.h"

#include "exceptions.h"

#include <sstream>

#include <stdint.h>
#include <stdio.h>

void
stl_model::load(const char* filename)
{
  auto* file = fopen(filename, "rb");
  if (!file) {
    std::ostringstream error;
    error << "Failed to open \"" << filename << "\"";
    throw stl_error(error.str());
  }

  auto emit_read_failure = [filename](const char* item) {
    std::ostringstream error;
    error << "Failed to read " << item << " from \"" << filename << "\"";
    throw stl_error(error.str());
  };

  fseek(file, 80, SEEK_SET);

  uint32_t num_tris = 0;

  if (fread(&num_tris, sizeof(num_tris), 1, file) != 1) {
    fclose(file);
    emit_read_failure("triangle count");
  }

  for (uint32_t i = 0; i < num_tris; i++) {

    fseek(file, 84 + i * 50 + 12, SEEK_SET);

    std::array<stl_tri::float3, 3> p;

    static_assert(sizeof(p) == 36, "float3 is not sized correctly.");

    if (fread(&p, sizeof(p), 1, file) != 1) {
      fclose(file);
      emit_read_failure("triangle");
    }

    tris.emplace_back(stl_tri{ p[0], p[1], p[2] });
  }

  fclose(file);
}
