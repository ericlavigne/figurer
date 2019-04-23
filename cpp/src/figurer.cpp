#include "figurer.hpp"

namespace figurer {

    Context::Context() : value_fn_{nullptr} {}

    Context::~Context() {}

    void Context::set_state_size(int state_size) { state_size_ = state_size; }

    void Context::set_actuation_size(int actuation_size) { actuation_size_ = actuation_size; }

    void Context::set_value_fn(std::function<double(std::vector<double>)> value_fn) {
        value_fn_ = value_fn;
    }

    double Context::apply_value_fn(std::vector<double> state) {
        return value_fn_(state);
    }
}
