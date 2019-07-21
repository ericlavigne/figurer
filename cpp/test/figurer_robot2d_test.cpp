#include "figurer.hpp"
#include "gtest/gtest.h"
#include "figurer_robot2d_example.hpp"

namespace {
    TEST(FigurerRobot2DTest, Robot2D) {
        figurer::Context context = figurer_robot2d_example::robot2d_context();
        context.figure_iterations(100);
        figurer::Plan plan = context.sample_plan();
        std::cout << plan << std::endl;

        //std::cout << context << std::endl;
        EXPECT_EQ(5, plan.actuations.size());
        EXPECT_EQ(6, plan.states.size());
        ASSERT_NEAR(figurer_robot2d_example::origin[0], plan.states[0][0], 0.01);
        ASSERT_NEAR(figurer_robot2d_example::origin[1], plan.states[0][1], 0.01);
        int last_state_index = plan.states.size() - 1;
        std::vector<double> last_state = plan.states[last_state_index];
        std::vector<double> goal = figurer_robot2d_example::goal;
        double dist = figurer::distance(last_state,goal);

        std::cout << "Robo2D reached: " << last_state[0] << "," << last_state[1]
                  << "\n          goal: " << goal[0] << "," << goal[1]
                  << "\n         error: " << dist << std::endl << std::endl
                  ;
        ASSERT_NEAR(last_state[0], goal[0], 1.0);
        ASSERT_NEAR(last_state[1], goal[1], 1.0);
    }
}
