#include "figurer_spatial_index.hpp"
#include "gtest/gtest.h"

namespace {
    TEST(FigurerSpatialIndexTest, SpatialIndex) {
        auto index = figurer::spatial_index(3);
        index.add(101, std::vector<double>{10,20,30});
        index.add(102, std::vector<double>{20,30,40});
        index.add(103, std::vector<double>{30,40,50});
        index.add(104, std::vector<double>{40,20,30});
        index.add(105, std::vector<double>{20,40,30});
        auto found = index.closest(std::vector<double>{41,19,29});
        EXPECT_EQ(104, found.first);
    }
}