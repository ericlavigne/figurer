#include "figurer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <iomanip>

namespace figurer {

    Context::Context() : value_fn_{nullptr}, policy_fn_{nullptr}, predict_fn_{nullptr},
        state_size_{-1}, actuation_size_{-1}, initial_state_node_id_{-1},
        max_state_node_id_{0}, max_distribution_node_id_{0}, depth_{-1},
        rootSpread_{-1}, avg_dist_sparsity_{-1},
        state_to_node_id_{},
        maxValueSoFar_{std::numeric_limits<double>::min() / 2.0},
        minValueSoFar_{std::numeric_limits<double>::max() / 2.0} {}

    Context::~Context() = default;

    void Context::set_state_size(int state_size) { state_size_ = state_size; }

    void Context::set_actuation_size(int actuation_size) { actuation_size_ = actuation_size; }

    void Context::set_depth(int depth) { depth_ = depth; }

    void Context::set_initial_state(std::vector<double> initial_state) {
        initial_state_ = move(initial_state);
    }

    void Context::set_value_fn(std::function<double(std::vector<double>)> value_fn) {
        value_fn_ = move(value_fn);
    }

    void Context::set_policy_fn(std::function<Distribution(std::vector<double>)> policy_fn) {
        policy_fn_ = move(policy_fn);
    }

    void Context::set_predict_fn(std::function<Distribution(std::vector<double>,std::vector<double>)> predict_fn) {
        predict_fn_ = move(predict_fn);
    }

    void Context::set_predict_inverse_fn(std::function<std::vector<double>(std::vector<double>,std::vector<double>)>
            predict_inverse_fn) {
        predict_inverse_fn_ = move(predict_inverse_fn);
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
        std::cout << "\n\n=== sample plan===\n\n";
        ensure_consistent_state();
        Plan plan;
        int state_node_id = initial_state_node_id_;
        plan.states.push_back(initial_state_);
        for(int i = 0; i < depth; i++) {
            std::cout << i << ": state node id " << state_node_id << std::endl;
            // Select actuation and next distribution node that maximize expected value.
            auto state_node = node_id_to_state_node_.at(state_node_id);
            double max_value = 0.0;
            int next_dist_id = -1;
            for(const auto& edge : state_node.next_distribution_nodes) {
                auto& next_node = node_id_to_distribution_node_.at(edge.second.distribution_node_id);
                if(next_dist_id < 0 || next_node.value > max_value) {
                    std::cout << "  dist node " << next_node.node_id << " has higher value of " << next_node.value << std::endl;
                    max_value = next_node.value;
                    next_dist_id = next_node.node_id;
                }
            }
            if(next_dist_id < 0) {
                return plan;
            }
            std::vector<double> actuation = state_node.next_distribution_nodes.at(next_dist_id).actuation;
            auto& dist_node = node_id_to_distribution_node_.at(next_dist_id);
            std::cout << "final dist node " << next_dist_id << " has actuation ";
            for(int j = 0; j < actuation.size(); j++) {
                if(j > 0) {
                    std::cout << ", ";
                }
                std::cout << actuation[j];
            }
            std::cout << std::endl << std::endl;

            // Select next state randomly because this step represents uncontrollable randomness in the world.
            if(dist_node.next_state_nodes.empty()) {
                return plan;
            }
            std::vector<std::pair<int,DistributionStateEdge>> sample_out;
            std::sample(dist_node.next_state_nodes.begin(),dist_node.next_state_nodes.end(),
                    std::back_inserter(sample_out), 1, std::mt19937{std::random_device{}()});
            state_node_id = sample_out[0].second.state_node_id;
            state_node = node_id_to_state_node_.at(state_node_id);

            std::cout << "sampled next state node " << state_node_id << " with state ";
            for(int j = 0; j < state_node.state.size(); j++) {
                if(j > 0) {
                    std::cout << ", ";
                }
                std::cout << state_node.state[j];
            }
            std::cout << std::endl << std::endl;

            // Add step to plan based on selected distribution and state nodes.
            plan.actuations.push_back(actuation);
            plan.states.push_back(state_node.state);
        }
        return plan;
    }

    void Context::ensure_consistent_state() {
        // initial state present
        if(this->initial_state_.empty()) {
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
        if(example_actuation.empty()) {
            throw std::invalid_argument("policy_fn yields empty actuation");
        }
        if(this->actuation_size_ > 0 && example_actuation.size() != this->actuation_size_) {
            throw std::invalid_argument("policy_fn yields actuation of size " + std::to_string(example_actuation.size()) +
                                        " which doesn't match expected size " + std::to_string(this->actuation_size_));
        }
        Distribution next_state_distribution = this->predict_fn_(this->initial_state_, example_actuation);
        std::vector<double> example_next_state = next_state_distribution.sample();
        if(example_next_state.empty()) {
            throw std::invalid_argument("predict_fn yields empty state");
        }
        if(this->state_size_ > 0 && example_next_state.size() != this->state_size_) {
            throw std::invalid_argument("predict_fn yields state of size " + std::to_string(example_next_state.size()) +
                                        " which doesn't match expected size " + std::to_string(this->state_size_));
        }
        // initial state node exists and matches initial state (otherwise fix by creating initial node)
        if(initial_state_node_id_ < 0 || node_id_to_state_node_[initial_state_node_id_].state != initial_state_) {
            StateNode initial_node{};
            initial_node.node_id = ++max_state_node_id_;
            initial_node.state = initial_state_;
            initial_node.direct_value = initial_value;
            initial_node.value = initial_value;
            initial_node.next_actuation_distribution = initial_policy;
            node_id_to_state_node_[initial_node.node_id] = initial_node;
            initial_state_node_id_ = initial_node.node_id;
            state_to_node_id_.add(initial_state_node_id_, initial_state_);
        }
    }

    double Context::default_sparsity_error_for_state_node() {
        if(rootSpread_ > 0) {
            return rootSpread_;
        } else if(maxValueSoFar_ > minValueSoFar_ + 1) {
            return maxValueSoFar_ - minValueSoFar_;
        }
        return 1000;
    }

    double Context::default_sparsity_error_for_distribution_node() {
        if(avg_dist_sparsity_ > 0) {
            return avg_dist_sparsity_;
        } else if(rootSpread_ > 0) {
            return rootSpread_;
        } else if(maxValueSoFar_ > minValueSoFar_ + 1) {
            return maxValueSoFar_ - minValueSoFar_;
        }
        return 1000;
    }

    void Context::refresh_state_node(int state_node_id) {
        auto& this_node = node_id_to_state_node_.at(state_node_id);
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
        for(auto& edge : this_node.next_distribution_nodes) {
            auto& next_node = node_id_to_distribution_node_.at(edge.second.distribution_node_id);
            if(total_paths == 0 || next_node.value > max_value) {
                max_value = next_node.value;
                max_value_depth = next_node.depth;
            }
            double value_plus_error = next_node.value + next_node.total_error;
            if(max_value_plus_error_id < 0 || value_plus_error < min_value_plus_error) {
                min_value_plus_error = value_plus_error;
            }
            if(max_value_plus_error_id < 0 || value_plus_error > max_value_plus_error) {
                second_max_value_plus_error = max_value_plus_error;
                second_max_value_plus_error_id = max_value_plus_error_id;
                max_value_plus_error = value_plus_error;
                max_value_plus_error_id = next_node.node_id;
            } else if(second_max_value_plus_error_id < 0 || next_node.value > max_value) {
                second_max_value_plus_error = value_plus_error;
                second_max_value_plus_error_id = next_node.node_id;
            }
            if(total_paths == 0 || next_node.value < min_value) {
                min_value = next_node.value;
            }
            double value_minus_error = next_node.value - next_node.total_error;
            if(total_paths == 0 || value_minus_error > max_value_minus_error) {
                max_value_minus_error = value_minus_error;
            }
            total_paths++;
        }
        if(total_paths > 0) {
            // Calculate value error bars based on children only (will add direct value later).
            int this_depth = max_value_depth + 1;
            double sparsity_error = total_paths < 2 ? default_sparsity_error_for_state_node() * this_depth / depth_
                    : std::max(0.01, (max_value - min_value)) / total_paths;
            double child_value_min = max_value_minus_error;
            double child_value_max1 = max_value_plus_error;
            double child_value_max2 = second_max_value_plus_error_id < 0 ? max_value_plus_error : second_max_value_plus_error;
            double child_value_max_floor = std::max(child_value_min, child_value_max2);
            double child_value_max = child_value_max_floor
                    + 0.1 * (child_value_max1 - child_value_max_floor);
            // Calculate value error bars including direct value.
            double final_value_min = (this_node.direct_value + this_depth * child_value_min)
                                     / (this_depth + 1);
            double final_value_max = (this_node.direct_value + this_depth * child_value_max)
                                     / (this_depth + 1);
            double final_value = (final_value_min + final_value_max) * 0.5;
            // Record stats in node.
            this_node.value = final_value;
            this_node.depth = this_depth;
            this_node.child_error = final_value - final_value_min;
            this_node.sparsity_error = sparsity_error;
            this_node.total_error = sqrt(pow(this_node.child_error,2.0) + pow(sparsity_error,2.0));
            if(total_paths > 2 && state_node_id == initial_state_node_id_) {
                rootSpread_ = max_value - min_value;
            }
        } else {
            this_node.value = this_node.direct_value;
            this_node.depth = 0;
            this_node.child_error = 0;
            this_node.sparsity_error = 0;
            this_node.total_error = 0;
        }
    }

    void Context::refresh_distribution_node(int distribution_node_id) {
        auto& this_node = node_id_to_distribution_node_.at(distribution_node_id);
        double total_value = 0.0;
        double min_child_value = 0.0;
        double max_child_value = 0.0;
        double total_child_error_squared = 0.0;
        int max_depth = 0;
        int total_paths = 0;
        for(auto& edge : this_node.next_state_nodes) {
            auto& next_node = node_id_to_state_node_.at(edge.second.state_node_id);
            double child_value = next_node.value;
            total_value += child_value;
            total_child_error_squared += pow(next_node.total_error, 2.0);
            if(total_paths == 0 || child_value < min_child_value) {
                min_child_value = child_value;
            }
            if(total_paths == 0 || child_value > max_child_value) {
                max_child_value = child_value;
            }
            if(next_node.depth > max_depth) {
                max_depth = next_node.depth;
            }
            total_paths++;
        }
        if(total_paths > 0) {
            double child_error = sqrt(total_child_error_squared) / std::max(total_paths, 1);
            double sparsity_error = (max_child_value - min_child_value + child_error) / std::max(total_paths, 1);
            if(total_paths < 2) {
                sparsity_error = default_sparsity_error_for_distribution_node();
            }
            double total_error = sqrt(pow(child_error, 2.0) + pow(sparsity_error, 2.0));
            this_node.value = total_value / total_paths;
            this_node.depth = max_depth;
            this_node.child_error = child_error;
            this_node.sparsity_error = sparsity_error;
            this_node.total_error = total_error;
            if(total_paths > 1) {
                double low_sparsity_estimate = std::max(0.01, (total_error - child_error) * total_paths);
                if(avg_dist_sparsity_ < 0) {
                    avg_dist_sparsity_ = low_sparsity_estimate;
                } else {
                    avg_dist_sparsity_ = 0.95 * avg_dist_sparsity_ + 0.05 * low_sparsity_estimate;
                }
            }
        } else {
            double sparsity_error = default_sparsity_error_for_distribution_node();
            this_node.value = 0;
            this_node.depth = 0;
            this_node.child_error = 0;
            this_node.sparsity_error = sparsity_error;
            this_node.total_error = sparsity_error;
        }
    }

    StateDistributionEdge Context::create_from_state_node(int state_node_id) {
        auto& state_node = node_id_to_state_node_.at(state_node_id);
        // Sample next actuation and resulting state distribution.
        std::vector<double> actuation = state_node.next_actuation_distribution.sample();
        Distribution next_state_distribution = this->predict_fn_(state_node.state, actuation);

        // Try to connect to nearby state instead of creating new
        if(predict_inverse_fn_) {
            auto next_state = next_state_distribution.sample();
            auto nearby = state_to_node_id_.closest(next_state);
            int nearby_state_node_id = nearby.first;
            // Ensure nearby isn't already a child before continuing connection effort.
            bool nearby_already_connected = false;
            for(auto& child_dist_edge : state_node.next_distribution_nodes) {
                auto& child_dist = node_id_to_distribution_node_.at(child_dist_edge.second.distribution_node_id);
                for(auto& child_state_edge : child_dist.next_state_nodes) {
                    if(nearby_state_node_id == child_state_edge.second.state_node_id) {
                        nearby_already_connected = true;
                    }
                }
            }
            if(! nearby_already_connected) {
                auto aim_actuation = predict_inverse_fn_(state_node.state, nearby.second);
                Distribution aim_state_dist = predict_fn_(state_node.state, aim_actuation);
                auto aim_state_sample = aim_state_dist.sample();
                double old_distance2 = distance2(next_state, nearby.second);
                double aim_distance2 = distance2(aim_state_sample, nearby.second);
                // If aiming can't get us at least 5x closer then don't bother.
                if(aim_distance2 < 0.04 * old_distance2) {
                    double next_policy_density = state_node.next_actuation_distribution.density(actuation);
                    double aim_policy_density = state_node.next_actuation_distribution.density(aim_actuation);
                    double next_actuation_distance = 1.0;
                    double aim_actuation_distance = 1.0;
                    if(!state_node.next_distribution_nodes.empty()) {
                        next_actuation_distance = state_node.actuations_so_far.closest_distance(actuation);
                        aim_actuation_distance = state_node.actuations_so_far.closest_distance(aim_actuation);
                    }
                    // Aim version is allowed to be up to 5x worse in combination of policy density
                    // and distance from other actuations.
                    if(aim_policy_density * next_actuation_distance > 0.2 * next_policy_density * aim_actuation_distance) {
                        // Finalize decision to aim by replacing actuation and state distribution with aim versions.
                        actuation = aim_actuation;
                        next_state_distribution = aim_state_dist;
                        //std::cout << "State" << state_node_id << " created dist" << this->max_distribution_node_id_ + 1
                        //          << " to aim at state" << nearby_state_node_id << std::endl;
                    }
                }
            }
        }

        // Create node and edge for new distribution node.
        int next_distribution_node_id = ++this->max_distribution_node_id_;
        StateDistributionEdge next_state_distribution_edge{};
        next_state_distribution_edge.state_node_id = state_node_id;
        next_state_distribution_edge.distribution_node_id = next_distribution_node_id;
        next_state_distribution_edge.actuation = actuation;
        DistributionNode next_distribution_node{};
        next_distribution_node.node_id = next_distribution_node_id;
        next_distribution_node.next_state_distribution = next_state_distribution;
        next_distribution_node.value = state_node.value;
        next_distribution_node.depth = 0;
        // Add node and edge to context and return edge.
        node_id_to_distribution_node_.emplace(next_distribution_node_id, next_distribution_node);
        state_node.next_distribution_nodes.emplace(next_distribution_node.node_id, next_state_distribution_edge);
        state_node.actuations_so_far.add(next_distribution_node_id,actuation);
        return next_state_distribution_edge;
    }

    StateDistributionEdge Context::create_or_explore_from_state_node(int state_node_id) {
        auto& state_node = node_id_to_state_node_.at(state_node_id);
        // Force some variety so that sparcity error can be estimated accurately.
        if(state_node.next_distribution_nodes.size() < 3) {
            return create_from_state_node(state_node_id);
        }
        // If sparcity error dominates then address that problem with new node.
        if(state_node.sparsity_error > state_node.child_error) {
            return create_from_state_node(state_node_id);
        }
        // Otherwise refine the most promising child node.
        double max_value_plus_error = 0;
        int max_value_plus_error_id = -1;
        for(auto& edge : state_node.next_distribution_nodes) {
            auto& next_node = node_id_to_distribution_node_.at(edge.second.distribution_node_id);
            double value_plus_error = next_node.value + next_node.total_error;
            if(max_value_plus_error_id < 0 || value_plus_error > max_value_plus_error) {
                max_value_plus_error_id = next_node.node_id;
                max_value_plus_error = value_plus_error;
            }
        }
        return state_node.next_distribution_nodes.at(max_value_plus_error_id);
    }

    DistributionStateEdge Context::create_from_distribution_node(int distribution_node_id) {
        // Sample state distribution to determine next state, then create next state node.
        auto& distribution_node = node_id_to_distribution_node_.at(distribution_node_id);
        std::vector<double> state = distribution_node.next_state_distribution.sample();
        double sample_density = distribution_node.next_state_distribution.density(state);
        DistributionStateEdge distribution_state_edge{};
        distribution_state_edge.distribution_node_id = distribution_node_id;

        // Try to connect to nearby state instead of creating new
        auto nearby = state_to_node_id_.closest(state);
        if(distribution_node.next_state_nodes.find(nearby.first) == distribution_node.next_state_nodes.end()) {
            double nearby_density = distribution_node.next_state_distribution.density(nearby.second);
            if(nearby_density > 0.1 * sample_density) {
                distribution_state_edge.state_node_id = nearby.first;
                distribution_state_edge.density = nearby_density;
                distribution_node.next_state_nodes.emplace(nearby.first, distribution_state_edge);
                //std::cout << "Distribution node " << distribution_node_id << " connecting to existing state node " << nearby.first << std::endl;
                return distribution_state_edge;
            }
        }

        int state_node_id = ++this->max_state_node_id_;
        distribution_state_edge.state_node_id = state_node_id;
        distribution_state_edge.density = sample_density;
        StateNode state_node{};
        state_node.node_id = state_node_id;
        state_node.state = state;
        state_node.next_actuation_distribution = this->policy_fn_(state);
        state_node.direct_value = this->value_fn_(state);
        state_node.value = state_node.direct_value;
        state_node.depth = 0;
        node_id_to_state_node_.emplace(state_node_id, state_node);
        distribution_node.next_state_nodes.emplace(state_node_id, distribution_state_edge);
        state_to_node_id_.add(state_node_id, state);
        if(state_node.direct_value > maxValueSoFar_) {
            maxValueSoFar_ = state_node.direct_value;
        }
        if(state_node.direct_value < minValueSoFar_) {
            minValueSoFar_ = state_node.direct_value;
        }
        return distribution_state_edge;
    }

    DistributionStateEdge Context::create_or_explore_from_distribution_node(int distribution_node_id) {
        auto& distribution_node = node_id_to_distribution_node_.at(distribution_node_id);
        // If no children then creating is the only option.
        if(distribution_node.next_state_nodes.empty()) {
            return create_from_distribution_node(distribution_node_id);
        }
        // If less than 2 children, sparsity error was set by default.
        // Better to use current default that is based on more data.
        double sparsity_error = distribution_node.next_state_nodes.size() < 2 ?
                default_sparsity_error_for_distribution_node() : distribution_node.sparsity_error;
        // If sparcity error dominates then address that problem with new node.
        if(sparsity_error > distribution_node.child_error) {
            return create_from_distribution_node(distribution_node_id);
        }
        // Otherwise refine the most promising child node.
        double max_value_plus_error = 0;
        int max_value_plus_error_id = -1;
        for(auto& edge : distribution_node.next_state_nodes) {
            auto& next_node = node_id_to_state_node_.at(edge.second.state_node_id);
            double value_plus_error = next_node.value + next_node.total_error;
            if(max_value_plus_error_id < 0 || value_plus_error > max_value_plus_error) {
                max_value_plus_error_id = next_node.node_id;
                max_value_plus_error = value_plus_error;
            }
        }
        return distribution_node.next_state_nodes.at(max_value_plus_error_id);
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

    std::ostream &operator<<(std::ostream &os, const figurer::Context &context) {
        std::ios_base::fmtflags oldflags = os.flags();
        std::streamsize oldprecision = os.precision();
        os << std::fixed << std::setprecision(2);
        os << "\n<Figurer::Context>\n";
        const figurer::StateNode &root = context.node_id_to_state_node_.at(context.initial_state_node_id_);
        os << "    initial: ";
        for (int i = 0; i < root.state.size(); i++) {
            if (i > 0) {
                os << ", ";
            }
            os << root.state[i];
        }
        os << "\n    value: " << root.value << " +/- " << root.total_error;
        os << "\n    sparsity: " << root.sparsity_error;
        os << "\n    children: " << root.next_distribution_nodes.size();
        os << "\n\n";
        for (auto &edge : root.next_distribution_nodes) {
            context.showStateDistEdge(os, edge.second, 2);
        }

        os << std::endl;
        os.flags(oldflags);
        os.precision(oldprecision);
        return os;
    }

    void Context::showStateDistEdge(std::ostream& os, const StateDistributionEdge& edge, int indent) const {
        auto &dist_node = node_id_to_distribution_node_.at(edge.distribution_node_id);
        for (int i = 0; i < indent; i++) {
            os << "    ";
        }
        os << "dist" << edge.distribution_node_id;
        os << "  act: ";
        for (int i = 0; i < edge.actuation.size(); i++) {
            if (i > 0) {
                os << ", ";
            }
            os << edge.actuation[i];
        }
        os << "  value: " << dist_node.value << " +/- " << dist_node.total_error
           << " (sparsity: " << dist_node.sparsity_error << ")";
        os << "\n";
        for (auto &next_edge : dist_node.next_state_nodes) {
            showDistStateEdge(os, next_edge.second, indent + 1);
        }
    }

    void Context::showDistStateEdge(std::ostream& os, const DistributionStateEdge& edge, int indent) const {
        auto &state_node = node_id_to_state_node_.at(edge.state_node_id);
        for(int i = 0; i < indent; i++) {
            os << "    ";
        }
        os << "state" << edge.state_node_id << "  ";
        for (int i = 0; i < state_node.state.size(); i++) {
            if (i > 0) {
                os << ", ";
            }
            os << state_node.state[i];
        }
        os << "  value: " << state_node.value << " +/- " << state_node.total_error
           << " (sparsity: " << state_node.sparsity_error << ")";

        if (indent < depth_*2) {
            os << "\n";
            for (auto &next_edge : state_node.next_distribution_nodes) {
                showStateDistEdge(os, next_edge.second, indent + 1);
            }
        } else {
            os << " ...\n";
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
