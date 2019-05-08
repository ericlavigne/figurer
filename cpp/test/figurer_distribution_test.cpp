#include "gtest/gtest.h"
#include "figurer_distribution.hpp"

namespace {
    TEST(FigurerDistributionTest, Uniform) {
        figurer::Distribution uniform = figurer::uniform_distribution({7.0, 9.0, -4.0, -1.0});
        EXPECT_EQ(0.0, uniform.density({8.0,1.0}));
        EXPECT_EQ(0.0, uniform.density({6.0,-3.0}));
        EXPECT_EQ(1.0, uniform.density({8.0,-3.0}));
        bool found_outside = false;
        bool found_low_x = false;
        bool found_high_x = false;
        bool found_low_y = false;
        bool found_high_y = false;
        for(int i = 0; i < 20; i++) {
            std::vector<double> s = uniform.sample();
            //std::cout << "sample: " << s[0] << ", " << s[1] << std::endl;
            if(s[0] < 7.0 || s[0] > 9.0 || s[1] < -4.0 || s[1] > -1.0) {
                found_outside = true;
            }
            if(s[0] < 7.5) {
                found_low_x = true;
            }
            if(s[0] > 8.5) {
                found_high_x = true;
            }
            if(s[1] < -3.5) {
                found_low_y = true;
            }
            if(s[1] > -1.5) {
                found_high_y = true;
            }
        }
        EXPECT_FALSE(found_outside) << "found value outside expected range";
        EXPECT_TRUE(found_low_x) << "didn't find low x";
        EXPECT_TRUE(found_high_x) << "didn't find high x";
        EXPECT_TRUE(found_low_y) << "didn't find low y";
        EXPECT_TRUE(found_high_y) << "didn't find high y";
    }
}
