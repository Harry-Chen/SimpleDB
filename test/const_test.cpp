#include "gtest/gtest.h"
#include "../src/constants.h"

TEST(PAGE_CONST, PAGE_CONST) {
  ASSERT_EQ(PAGE_SIZE, 8192);
  ASSERT_EQ(PAGE_SIZE, 1 << PAGE_IDX);
}
