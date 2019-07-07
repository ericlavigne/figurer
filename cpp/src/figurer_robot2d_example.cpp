#include "figurer_robot2d_example.hpp"

/*
 * Robot2D: Simple example of how to use figurer
 *
 * A robot moves on a 2D plane starting at (1,2) and trying to reach (3,4).
 * The robot can move by up to 1.0 along each dimension per time step.
 * Robot actuation is also 2D, representing how much the robot intends to
 * change each coordinate during this time step. Actual motion differs
 * randomly from the intended motion by +- 10% of the actuation or by
 * +- 0.01 absolute, whichever is greater.
 */

namespace figurer_robot2d_example {

    std::vector<double> origin {1,2};
    std::vector<double> goal {3,4};

    // The value returned by value_fn should be higher when state is closer to the goal.
    double value_fn(std::vector<double> state) {
        double origin_to_goal = fabs(origin[0] - goal[0]) + fabs(origin[1] - goal[1]);
        double state_to_goal = fabs(state[0] - goal[0]) + fabs(state[1] - goal[1]);
        return origin_to_goal - state_to_goal;
    }

    // Policy should hint at the most likely actuation to help us achieve our goal.
    // We can easily choose a policy that goes straight toward the goal because
    // this is an easy problem. To keep the work interesting in this example, we'll
    // instead choose a uniform distribution over all possible actuations.
    figurer::Distribution policy_fn(std::vector<double> state) {
        return figurer::uniform_distribution({-1.0,1.0,-1.0,1.0});
    }

    /*
     * Predict should return a probability distribution of next states that could result from
     * applying the actuation to this state. Actuation in this example is defined to be the
     * intended change in position. Actuation should always be in the range [-1,1] for each
     * dimension. If actuation is not in this expected range, predict will replace the given
     * extreme value with the most extreme allowed value in that direction. The next state
     * distribution is uniformly distributed around the intended next state, over a range of
     * +- 10% of the actuation or 0.01, whichever is larger.
     */
    figurer::Distribution predict_fn(std::vector<double> state, std::vector<double> actuation) {
        double new_x = state[0] + std::min(1.0, std::max(-1.0, actuation[0]));
        double new_y = state[1] + std::min(1.0, std::max(-1.0, actuation[1]));
        double uncertainty_x = std::max(0.01, fabs(actuation[0] * 0.1));
        double uncertainty_y = std::max(0.01, fabs(actuation[1] * 0.1));
        return figurer::uniform_distribution({new_x - uncertainty_x, new_x + uncertainty_x,
                                              new_y - uncertainty_y, new_y + uncertainty_y});
    }

    /*
     * Predict-inverse chooses an actuation from state1 that will give a next state as close
     * as possible to state2. An actuation that exactly achieves that transition would be
     * (state2 - state1) so we'll start with that and constrain each dimension to the
     * required [-1,1] range.
     */
    std::vector<double> predict_inverse_fn(std::vector<double> state1, std::vector<double> state2) {
        std::vector<double> result(2,0.0);
        return {std::min(1.0, std::max(-1.0, state2[0] - state1[0])),
                std::min(1.0, std::max(-1.0, state2[1] - state1[1]))};
    }

    figurer::Context robot2d_context() {
        figurer::Context context;
        context.set_state_size(2);
        context.set_actuation_size(2);
        context.set_depth(5);
        context.set_initial_state(origin);
        context.set_value_fn(value_fn);
        context.set_policy_fn(policy_fn);
        context.set_predict_fn(predict_fn);
        context.set_predict_inverse_fn(predict_inverse_fn);
        return context;
    }
}
