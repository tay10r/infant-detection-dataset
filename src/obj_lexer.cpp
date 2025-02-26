#include "obj_lexer.h"

namespace {

auto
is_space(const char c) -> bool
{
  return (c == ' ') || (c == '\t') || (c == '\r') || (c == '\n');
}

} // namespace

obj_lexer::obj_lexer(const std::string_view& source)
  : source_(source)
{
  skip_space();
}

auto
obj_lexer::done() const -> bool
{
  return offset_ >= source_.size();
}

auto
obj_lexer::scan() -> obj_token
{
  skip_space();

  size_t len = 0;

  while (((offset_ + len) < source_.size()) && !is_space(source_.at(offset_ + len))) {
    len++;
  }

  auto tok = produce(len);

  // We do not have to do this, but it will give the caller
  // a better indication of whether or not we've reached the
  // end of the file.
  skip_space();

  return tok;
}

void
obj_lexer::update_line_column(const char c)
{
  if (c == '\n') {
    line_++;
    column_ = 1;
  } else {
    column_++;
  }
}

auto
obj_lexer::produce(const size_t len) -> obj_token
{
  const auto data = source_.substr(offset_, len);

  obj_token tok{ data, line_, column_ };

  for (size_t i = 0; i < len; i++) {
    const auto c = source_.at(offset_ + i);
    update_line_column(c);
  }

  offset_ += len;

  return tok;
}

void
obj_lexer::skip_space()
{
  while (!done() && is_space(source_.at(offset_))) {
    update_line_column(source_.at(offset_));
    offset_++;
  }
}
