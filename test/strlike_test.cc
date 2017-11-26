#include "gtest/gtest.h"

bool strlike(const char* a, const char *b);

TEST(STRLIKE, STRLIKE_GENERAL) {
  ASSERT_TRUE(strlike("abc", "abc"));
  ASSERT_FALSE(strlike("abc", "abd"));
  ASSERT_TRUE(strlike("abc", "a_c"));
  ASSERT_TRUE(strlike("abc", "a%%"));
  ASSERT_TRUE(strlike("a%c", "a\\%%c"));
  ASSERT_FALSE(strlike("acc", "a\\%%c"));
  ASSERT_TRUE(strlike("ab", "a[a-c]"));
  ASSERT_FALSE(strlike("ab", "a[!a-c]"));
  ASSERT_TRUE(strlike("ae", "a[!a-c]"));
}
