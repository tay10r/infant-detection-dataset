#pragma once

#include <string_view>

struct obj_token final
{
  std::string_view data;
  size_t line;
  size_t column;
};

class obj_lexer final
{
public:
  obj_lexer(const std::string_view& source);

  auto done() const -> bool;

  auto scan() -> obj_token;

protected:
  auto produce(size_t len) -> obj_token;

  void update_line_column(const char c);

  void skip_space();

private:
  std::string_view source_;

  size_t offset_{};

  size_t line_{ 1 };

  size_t column_{ 1 };
};
