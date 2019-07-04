#ifndef FIGURER_HPP
#define FIGURER_HPP

#include "figurer_distribution.hpp"
#include <functional>
#include <unordered_map>
#include <vector>

namespace figurer {

    struct Plan {
        std::vector<std::vector<double>> states;
        std::vector<std::vector<double>> actuations;
    };

    struct StateDistributionEdge {
        int state_node_id;
        int distribution_node_id;
        std::vector<double> actuation;
    };

    struct DistributionStateEdge {
        int distribution_node_id;
        int state_node_id;
        double density;
    };

    struct StateNode {
        int node_id;
        std::vector<double> state;
        Distribution next_actuation_distribution;
        int visits;
        double direct_value;
        double value;
        int depth;
        std::unordered_map<int,StateDistributionEdge> next_distribution_nodes;
    };

    struct DistributionNode {
        int node_id;
        Distribution next_state_distribution;
        double value;
        int depth;
        std::unordered_map<int,DistributionStateEdge> next_state_nodes;
    };

    class Context {
        // Number of elements in state vector. Set to -1 to skip validation.
        int state_size_;
        // Number of elements in actuation vector. Set to -1 to skip validation.
        int actuation_size_;
        // Depth of lookahead
        int depth_;
        std::vector<double> initial_state_;
        // value: (state)->value
        std::function<double(std::vector<double>)> value_fn_;
        // policy: (state)->actuation dist
        std::function<Distribution(std::vector<double>)> policy_fn_;
        // predict: (state,actuation)->state dist
        std::function<Distribution(std::vector<double>,std::vector<double>)> predict_fn_;
        // predict inverse: (state1,state2)->actuation
        // What actuation should be used from state1 if the goal is to reach state2?
        // If state2 is not feasible or if the process is non-deterministic, then
        // actuation should be selected to come as close as possible.
        std::function<std::vector<double>(std::vector<double>,std::vector<double>)> predict_inverse_fn_;
        void ensure_consistent_state();
        // figure_once takes a small step toward solving the optimization problem.
        void figure_once();
        void refresh_state_node(int state_node_id);
        void refresh_distribution_node(int distribution_node_id);
        std::unordered_map<int,StateNode> node_id_to_state_node_;
        int initial_state_node_id_;
        int max_state_node_id_;
        std::unordered_map<int,DistributionNode> node_id_to_distribution_node_;
        int max_distribution_node_id_;
    public:
        Context();
        ~Context();
        void set_state_size(int state_size);
        void set_actuation_size(int actuation_size);
        void set_depth(int depth);
        void set_initial_state(std::vector<double> initial_state);
        void set_value_fn(std::function<double(std::vector<double>)> value_fn);
        void set_policy_fn(std::function<Distribution(std::vector<double>)> policy_fn);
        void set_predict_fn(std::function<Distribution(std::vector<double>,std::vector<double>)> predict_fn);
        void set_predict_inverse_fn(std::function<std::vector<double>(std::vector<double>,std::vector<double>)> predict_inverse_fn);

        void figure_seconds(double seconds);
        void figure_iterations(int iterations);
        Plan sample_plan();
        Plan sample_plan(int depth);
    };

}

std::ostream& operator<<(std::ostream& os, const figurer::Plan& plan);

#endif
