#include <gtest/gtest.h>

#include "obj_lexer.h"

TEST(obj_lexer, scan_eof)
{
  obj_lexer lexer("");
  EXPECT_EQ(lexer.done(), true);
}

TEST(obj_lexer, scan_space)
{
  obj_lexer lexer(" ");
  EXPECT_EQ(lexer.done(), true);
  const auto tok = lexer.scan();
  EXPECT_EQ(tok.data.empty(), true);
  EXPECT_EQ(lexer.done(), true);
}

TEST(obj_lexer, scan_tokens)
{
  obj_lexer lexer(" a\nb ");

  EXPECT_EQ(lexer.done(), false);

  const auto a = lexer.scan();
  EXPECT_EQ(a.data, "a");
  EXPECT_EQ(a.line, 1);
  EXPECT_EQ(a.column, 2);

  EXPECT_EQ(lexer.done(), false);

  const auto b = lexer.scan();
  EXPECT_EQ(b.data, "b");
  EXPECT_EQ(b.line, 2);
  EXPECT_EQ(b.column, 1);

  EXPECT_EQ(lexer.done(), true);
}
