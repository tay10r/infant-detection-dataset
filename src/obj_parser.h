#pragma once

#include <string_view>

struct obj_model;

[[nodiscard]] auto
parse_obj(const std::string_view& source) -> obj_model;

[[nodiscard]] auto
parse_obj_file(const char* filename) -> obj_model;
