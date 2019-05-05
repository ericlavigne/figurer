#include "gtest/gtest.h"
#include "figurer.hpp"

namespace {
    TEST(FigurerTest, ValueFn) {
        figurer::Context context{};
        context.set_actuation_size(2);
        context.set_state_size(4);
        context.set_value_fn([](std::vector<double> state){ return 7.0; });
        context.figure_seconds(0.0001);
        EXPECT_EQ(7.0,context.apply_value_fn(std::vector<double>{1.0,2.0,3.0,4.0}));
    }
}
