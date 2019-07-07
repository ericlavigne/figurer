#include "figurer.hpp"
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <iomanip>

namespace figurer {

    Context::Context() : value_fn_{nullptr}, policy_fn_{nullptr}, predict_fn_{nullptr},
        state_size_{-1}, actuation_size_{-1}, initial_state_node_id_{-1}, max_state_node_id_{0},
        rootSpread_{-1}, maxValueSoFar_{std::numeric_limits<double>::min() / 2.0},
        minValueSoFar_{std::numeric_limits<double>::max() / 2.0} {}

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
        this->ensure_consistent_state();
        while(true) {
            figure_once();
            auto current_time = std::chrono::high_resolution_clock::now();
            if(current_time - start_time > duration) {
                return;
            }
        }
    }

    void Context::figure_iterations(int iterations) {
        this->ensure_consistent_state();
        for(int i=0; i<iterations; i++) {
            figure_once();
        }
    }

    Plan Context::sample_plan() {
        return sample_plan(depth_);
    }

    Plan Context::sample_plan(int depth) {
        this->ensure_consistent_state();
        Plan plan;
        int state_node_id = initial_state_node_id_;
        plan.states.push_back(initial_state_);
        for(int i = 0; i < depth; i++) {
            // Select actuation and next distribution node that maximize expected value.
            auto state_node = node_id_to_state_node_.at(state_node_id);
            double max_value = 0.0;
            int next_dist_id = -1;
            for(auto edge : state_node.next_distribution_nodes) {
                auto next_node = node_id_to_distribution_node_.find(edge.second.distribution_node_id);
                if(next_dist_id < 0 || next_node->second.value > max_value) {
                    max_value = next_node->second.value;
                    next_dist_id = next_node->second.node_id;
                }
            }
            if(next_dist_id < 0) {
                return plan;
            }
            std::vector<double> actuation = state_node.next_distribution_nodes.find(next_dist_id)->second.actuation;
            auto dist_node = node_id_to_distribution_node_.at(next_dist_id);

            // Select next state randomly because this step represents uncontrollable randomness in the world.
            if(dist_node.next_state_nodes.empty()) {
                return plan;
            }
            std::vector<std::pair<int,DistributionStateEdge>> sample_out;
            std::sample(dist_node.next_state_nodes.begin(),dist_node.next_state_nodes.end(),
                    std::back_inserter(sample_out), 1, std::mt19937{std::random_device{}()});
            state_node_id = sample_out[0].second.state_node_id;
            state_node = node_id_to_state_node_.at(state_node_id);

            // Add step to plan based on selected distribution and state nodes.
            plan.actuations.push_back(actuation);
            plan.states.push_back(state_node.state);
        }
        return plan;
    }

    void Context::ensure_consistent_state() {
        // initial state present
        if(this->initial_state_.size() == 0) {
            throw std::invalid_argument("Initial state is empty");
        }
        if(this->state_size_ > 0 && this->initial_state_.size() != this->state_size_) {
            throw std::invalid_argument("State size " + std::to_string(this->initial_state_.size()) +
                                        " doesn't match expected size " + std::to_string(this->state_size_));
        }
        // callbacks present
        if(this->value_fn_ == nullptr) {
            throw std::invalid_argument("Missing value_fn");
        }
        if(this->policy_fn_ == nullptr) {
            throw std::invalid_argument("Missing policy_fn");
        }
        if(this->predict_fn_ == nullptr) {
            throw std::invalid_argument("Missing predict_fn");
        }
        // callback dimensions consistent (call each once)
        double initial_value = this->value_fn_(this->initial_state_);
        if(initial_value > maxValueSoFar_) {
            maxValueSoFar_ = initial_value;
        }
        if(initial_value < minValueSoFar_) {
            minValueSoFar_ = initial_value;
        }
        Distribution initial_policy = this->policy_fn_(this->initial_state_);
        std::vector<double> example_actuation = initial_policy.sample();
        if(example_actuation.size() == 0) {
            throw std::invalid_argument("policy_fn yields empty actuation");
        }
        if(this->actuation_size_ > 0 && example_actuation.size() != this->actuation_size_) {
            throw std::invalid_argument("policy_fn yields actuation of size " + std::to_string(example_actuation.size()) +
                                        " which doesn't match expected size " + std::to_string(this->actuation_size_));
        }
        Distribution next_state_distribution = this->predict_fn_(this->initial_state_, example_actuation);
        std::vector<double> example_next_state = next_state_distribution.sample();
        if(example_next_state.size() == 0) {
            throw std::invalid_argument("predict_fn yields empty state");
        }
        if(this->state_size_ > 0 && example_next_state.size() != this->state_size_) {
            throw std::invalid_argument("predict_fn yields state of size " + std::to_string(example_next_state.size()) +
                                        " which doesn't match expected size " + std::to_string(this->state_size_));
        }
        // initial state node exists and matches initial state (otherwise fix by creating initial node)
        if(initial_state_node_id_ < 0 || node_id_to_state_node_[initial_state_node_id_].state != initial_state_) {
            StateNode initial_node;
            initial_node.node_id = ++max_state_node_id_;
            initial_node.state = initial_state_;
            initial_node.direct_value = initial_value;
            initial_node.value = initial_value;
            initial_node.next_actuation_distribution = initial_policy;
            node_id_to_state_node_[initial_node.node_id] = initial_node;
            initial_state_node_id_ = initial_node.node_id;
        }
    }

    double Context::default_sparsity_error() {
        if(rootSpread_ > 0) {
            return rootSpread_;
        } else if(maxValueSoFar_ > minValueSoFar_ + 1000) {
            return maxValueSoFar_ - minValueSoFar_;
        }
        return 1000;
    }

    void Context::refresh_state_node(int state_node_id) {
        auto this_node = node_id_to_state_node_.at(state_node_id);
        double max_value = 0.0;
        double min_value_plus_error = 0.0;
        double max_value_plus_error = 0.0;
        int max_value_plus_error_id = -1;
        double second_max_value_plus_error = 0.0;
        int second_max_value_plus_error_id = -1;
        double min_value = 0.0;
        double max_value_minus_error = 0.0;
        int max_value_depth = 0;
        int total_paths = 0;
        for(auto edge : this_node.next_distribution_nodes) {
            auto next_node = node_id_to_distribution_node_.find(edge.second.distribution_node_id);
            if(total_paths == 0 || next_node->second.value > max_value) {
                max_value = next_node->second.value;
                max_value_depth = next_node->second.depth;
            }
            double value_plus_error = next_node->second.value + next_node->second.total_error;
            if(max_value_plus_error_id < 0 || value_plus_error < min_value_plus_error) {
                min_value_plus_error = value_plus_error;
            }
            if(max_value_plus_error_id < 0 || value_plus_error > max_value_plus_error) {
                second_max_value_plus_error = max_value_plus_error;
                second_max_value_plus_error_id = max_value_plus_error_id;
                max_value_plus_error = value_plus_error;
                max_value_plus_error_id = next_node->second.node_id;
            } else if(second_max_value_plus_error_id < 0 || next_node->second.value > max_value) {
                second_max_value_plus_error = value_plus_error;
                second_max_value_plus_error_id = next_node->second.node_id;
            }
            if(total_paths == 0 || next_node->second.value < min_value) {
                min_value = next_node->second.value;
            }
            double value_minus_error = next_node->second.value - next_node->second.total_error;
            if(total_paths == 0 || value_minus_error > max_value_minus_error) {
                max_value_minus_error = value_minus_error;
            }
            total_paths++;
        }
        if(total_paths > 0) {
            // Calculate value error bars based on children only (will add direct value later).
            double sparsity_error = total_paths < 2 ? default_sparsity_error()
                    : std::max(0.01, (max_value - min_value_plus_error)) / total_paths;
            double child_value_min = max_value_minus_error;
            double child_value_max1 = max_value_plus_error;
            double child_value_max2 = second_max_value_plus_error_id < 0 ? max_value_plus_error : second_max_value_plus_error;
            double child_value_max_floor = std::max(child_value_min, child_value_max2);
            double child_value_max = child_value_max_floor
                    + 0.1 * (child_value_max1 - child_value_max_floor)
                    + sparsity_error * 2;
            // Calculate value error bars including direct value.
            int this_depth = max_value_depth + 1;
            double final_value_min = (this_node.direct_value + this_depth * child_value_min)
                                     / (this_depth + 1);
            double final_value_max = (this_node.direct_value + this_depth * child_value_max)
                                     / (this_depth + 1);
            double final_value = (final_value_min + final_value_max) * 0.5;
            double total_error = final_value - final_value_min;
            double child_error = std::max(0.0, total_error - sparsity_error);
            // Record stats in node.
            node_id_to_state_node_.at(state_node_id).value = final_value;
            node_id_to_state_node_.at(state_node_id).depth = this_depth;
            node_id_to_state_node_.at(state_node_id).child_error = child_error;
            node_id_to_state_node_.at(state_node_id).sparsity_error = sparsity_error;
            node_id_to_state_node_.at(state_node_id).total_error = total_error;
            if(total_paths > 2 && state_node_id == initial_state_node_id_) {
                rootSpread_ = max_value - min_value;
            }
        }
    }

    void Context::refresh_distribution_node(int distribution_node_id) {
        auto this_node = node_id_to_distribution_node_.at(distribution_node_id);
        double total_value = 0.0;
        double min_child_value = 0.0;
        double max_child_value = 0.0;
        double total_child_error_squared = 0.0;
        int max_depth = 0;
        int total_paths = 0;
        for(auto edge : this_node.next_state_nodes) {
            auto next_node = node_id_to_state_node_.find(edge.second.state_node_id);
            double child_value = next_node->second.value;
            total_value += child_value;
            total_child_error_squared += pow(next_node->second.total_error, 2.0);
            if(total_paths == 0 || child_value < min_child_value) {
                min_child_value = child_value;
            }
            if(total_paths == 0 || child_value > max_child_value) {
                max_child_value = child_value;
            }
            if(next_node->second.depth > max_depth) {
                max_depth = next_node->second.depth;
            }
            total_paths++;
        }
        if(total_paths > 0) {
            double child_error = sqrt(total_child_error_squared);
            double sparsity_error = (max_child_value - min_child_value + child_error) / std::max(total_paths, 1);
            if(total_paths < 2) {
                sparsity_error = default_sparsity_error();
            }
            double total_error = sqrt(pow(child_error, 2.0) + pow(sparsity_error, 2.0));
            node_id_to_distribution_node_.at(distribution_node_id).value = total_value / total_paths;
            node_id_to_distribution_node_.at(distribution_node_id).depth = max_depth;
            node_id_to_distribution_node_.at(distribution_node_id).child_error = child_error;
            node_id_to_distribution_node_.at(distribution_node_id).sparsity_error = sparsity_error;
            node_id_to_distribution_node_.at(distribution_node_id).total_error = total_error;
        } else {
            double sparsity_error = default_sparsity_error();
            node_id_to_distribution_node_.at(distribution_node_id).value = 0;
            node_id_to_distribution_node_.at(distribution_node_id).depth = 0;
            node_id_to_distribution_node_.at(distribution_node_id).child_error = 0;
            node_id_to_distribution_node_.at(distribution_node_id).sparsity_error = sparsity_error;
            node_id_to_distribution_node_.at(distribution_node_id).total_error = sparsity_error;
        }
    }

    StateDistributionEdge Context::create_or_explore_from_state_node(int state_node_id) {
        auto state_node = node_id_to_state_node_.find(state_node_id);
        // Should decide to improve stats on old path or start new path. For now always start new path.
        // Sample next actuation and resulting state distribution.
        std::vector<double> actuation = state_node->second.next_actuation_distribution.sample();
        Distribution next_state_distribution = this->predict_fn_(state_node->second.state, actuation);
        // Create node and edge for new distribution node.
        int next_distribution_node_id = ++this->max_distribution_node_id_;
        StateDistributionEdge next_state_distribution_edge;
        next_state_distribution_edge.state_node_id = state_node_id;
        next_state_distribution_edge.distribution_node_id = next_distribution_node_id;
        next_state_distribution_edge.actuation = actuation;
        DistributionNode next_distribution_node;
        next_distribution_node.node_id = next_distribution_node_id;
        next_distribution_node.next_state_distribution = next_state_distribution;
        next_distribution_node.value = state_node->second.value;
        next_distribution_node.depth = 0;
        node_id_to_distribution_node_.emplace(next_distribution_node_id, next_distribution_node);
        state_node->second.next_distribution_nodes.emplace(next_distribution_node.node_id,
                                                                   next_state_distribution_edge);
        return next_state_distribution_edge;
    }

    DistributionStateEdge Context::create_or_explore_from_distribution_node(int distribution_node_id) {
        // Sample state distribution to determine next state, then create next state node.
        auto distribution_node = node_id_to_distribution_node_.find(distribution_node_id);
        std::vector<double> state = distribution_node->second.next_state_distribution.sample();
        DistributionStateEdge distribution_state_edge;
        distribution_state_edge.distribution_node_id = distribution_node_id;
        int state_node_id = ++this->max_state_node_id_;
        distribution_state_edge.state_node_id = state_node_id;
        distribution_state_edge.density = distribution_node->second.next_state_distribution.density(state);
        StateNode state_node;
        state_node.node_id = state_node_id;
        state_node.state = state;
        state_node.next_actuation_distribution = this->policy_fn_(state);
        state_node.direct_value = this->value_fn_(state);
        state_node.value = state_node.direct_value;
        state_node.depth = 0;
        node_id_to_state_node_.emplace(state_node_id, state_node);
        distribution_node->second.next_state_nodes.emplace(state_node_id, distribution_state_edge);
        if(state_node.direct_value > maxValueSoFar_) {
            maxValueSoFar_ = state_node.direct_value;
        }
        if(state_node.direct_value < minValueSoFar_) {
            minValueSoFar_ = state_node.direct_value;
        }
        return distribution_state_edge;
    }

    void Context::figure_once() {
        int current_state_node_id = initial_state_node_id_;
        std::vector<int> visited_state_nodes {current_state_node_id};
        std::vector<int> visited_distribution_nodes;
        for(int depth = 0; depth < this->depth_; depth++) {
            // Create new nodes or refine existing nodes
            StateDistributionEdge state_distribution_edge = create_or_explore_from_state_node(current_state_node_id);
            DistributionStateEdge distribution_state_edge = create_or_explore_from_distribution_node(state_distribution_edge.distribution_node_id);

            current_state_node_id = distribution_state_edge.state_node_id;

            // Record visited nodes so their value can be updated later.
            visited_distribution_nodes.push_back(state_distribution_edge.distribution_node_id);
            visited_state_nodes.push_back(distribution_state_edge.state_node_id);
        }
        // Update value for all visited nodes.
        for(int depth = this->depth_ - 1; depth >= 0; depth--) {
            refresh_distribution_node(visited_distribution_nodes[depth]);
            refresh_state_node(visited_state_nodes[depth]);
        }
    }
}

std::ostream& operator<<(std::ostream& os, const figurer::Plan& plan) {
    std::ios_base::fmtflags oldflags = os.flags();
    std::streamsize oldprecision = os.precision();
    os << std::fixed << std::setprecision(2);
    os << "\n<Figurer::Plan>\n";
    for(int j = 0; j < plan.states[0].size(); j++) {
        if(j > 0) {
            os << ", ";
        }
        os << plan.states[0][j];
    }
    os << std::endl;
    for(int i = 0; i < plan.actuations.size(); i++) {
        for(int j = 0; j < plan.actuations[i].size(); j++) {
            if(j > 0) {
                os << ", ";
            }
            os << plan.actuations[i][j];
        }
        os << "  =>  ";
        for(int j = 0; j < plan.states[i+1].size(); j++) {
            if(j > 0) {
                os << ", ";
            }
            os << plan.states[i+1][j];
        }
        os << std::endl;
    }
    os << std::endl;
    os.flags(oldflags);
    os.precision(oldprecision);
    return os;
}
