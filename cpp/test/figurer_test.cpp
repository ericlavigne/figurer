#include "gtest/gtest.h"
#include "figurer.hpp"

namespace {
    TEST(FigurerTest, Equality) {
        EXPECT_EQ(3,figurer::figure());
    }
}
