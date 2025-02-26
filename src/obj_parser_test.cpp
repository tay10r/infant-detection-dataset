#include <gtest/gtest.h>

#include "obj_model.h"
#include "obj_parser.h"

TEST(obj_parser, parse_empty)
{
  const auto m = parse_obj("");
  EXPECT_TRUE(m.vertices.empty());
  EXPECT_TRUE(m.faces.empty());
}

TEST(obj_parser, parse)
{
  const char* source = "v 0.0 1.0 -2.0\n"
                       "f 1/2/3 4/5/6 7/8/9\n";
  const auto m = parse_obj(source);

  ASSERT_EQ(m.vertices.size(), 1);
  const auto v = m.vertices.at(0);
  EXPECT_NEAR(v.at(0), 0.0, 1.0e-3F);
  EXPECT_NEAR(v.at(1), 1.0, 1.0e-3F);
  EXPECT_NEAR(v.at(2), -2.0, 1.0e-3F);

  ASSERT_EQ(m.faces.size(), 1);
  const auto f = m.faces[0];
  EXPECT_EQ(f.position.at(0), 0);
  EXPECT_EQ(f.position.at(1), 3);
  EXPECT_EQ(f.position.at(2), 6);
  EXPECT_EQ(f.normal.at(0), 2);
  EXPECT_EQ(f.normal.at(1), 5);
  EXPECT_EQ(f.normal.at(2), 8);
}
