#ifndef FIGURER_ROBOT2D_EXAMPLE_HPP
#define FIGURER_ROBOT2D_EXAMPLE_HPP

#include "figurer.hpp"
#include <vector>

namespace figurer_robot2d_example {
    extern std::vector<double> origin;
    extern std::vector<double> goal;
    double value_fn(std::vector<double> state);
    figurer::Distribution policy_fn(std::vector<double> state);
    figurer::Distribution predict_fn(std::vector<double> state, std::vector<double> actuation);
    std::vector<double> predict_inverse_fn(std::vector<double> state1, std::vector<double> state2);
    figurer::Context robot2d_context();
}

#endif
