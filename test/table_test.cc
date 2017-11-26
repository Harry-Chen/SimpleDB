#include "gtest/gtest.h"
#include "../src/backend/Table.h"

TEST(TABLE_TEST, TABLE_TEST_SIZE) {
  ASSERT_LE(sizeof(TableHead), PAGE_SIZE);
}

TEST(TABLE_TEST, TABLE_TEST_CREATE) {

}
