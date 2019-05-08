#include "figurer.hpp"
#include <chrono>
#include <cmath>
#include <iostream>

namespace figurer {

    Context::Context() : value_fn_{nullptr}, policy_fn_{nullptr}, predict_fn_{nullptr},
        state_size_{-1}, actuation_size_{-1} {}

    Context::~Context() {}

    void Context::set_state_size(int state_size) { state_size_ = state_size; }

    void Context::set_actuation_size(int actuation_size) { actuation_size_ = actuation_size; }

    void Context::set_depth(int depth) { depth_ = depth; }

    void Context::set_initial_state(std::vector<double> initial_state) {
        initial_state_ = initial_state;
    }

    void Context::set_value_fn(std::function<double(std::vector<double>)> value_fn) {
        value_fn_ = value_fn;
    }

    void Context::set_policy_fn(std::function<Distribution(std::vector<double>)> policy_fn) {
        policy_fn_ = policy_fn;
    }

    void Context::set_predict_fn(std::function<Distribution(std::vector<double>,std::vector<double>)> predict_fn) {
        predict_fn_ = predict_fn;
    }

    void Context::set_predict_inverse_fn(std::function<std::vector<double>(std::vector<double>,std::vector<double>)>
            predict_inverse_fn) {
        predict_inverse_fn_ = predict_inverse_fn;
    }

    void Context::figure_seconds(double seconds) {
        auto duration = std::chrono::microseconds((long) (seconds * pow(10,6)));
        auto start_time = std::chrono::high_resolution_clock::now();
        while(true) {
            figure_once();
            auto current_time = std::chrono::high_resolution_clock::now();
            if(current_time - start_time > duration) {
                std::cout << std::endl;
                return;
            }
        }
    }

    void Context::figure_iterations(int iterations) {
        for(int i=0; i<iterations; i++) {
            figure_once();
        }
        std::cout << std::endl;
    }

    Plan Context::sample_plan() {
        return sample_plan(depth_);
    }

    Plan Context::sample_plan(int depth) {
        Plan plan;
        plan.states.push_back(initial_state_);
        return plan;
    }

    double Context::apply_value_fn(std::vector<double> state) {
        return value_fn_(state);
    }

    void Context::figure_once() {
        std::cout << ".";
    }
}
