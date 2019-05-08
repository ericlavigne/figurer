#ifndef FIGURER_DISTRIBUTION_HPP
#define FIGURER_DISTRIBUTION_HPP

#include <functional>
#include <vector>

namespace figurer {

    class Distribution {
        int dimension_;
        int seed_dimension_;
        std::function<std::vector<double>(std::vector<double>)> sample_fn_;
        std::function<double(std::vector<double>)> density_fn_;
    public:
        Distribution();
        void set_dimension(int dimension);
        void set_seed_dimension(int seed_dimension);
        void set_sample_fn(std::function<std::vector<double>(std::vector<double>)> sample_fn);
        void set_density_fn(std::function<double(std::vector<double>)> density_fn);
        std::vector<double> sample();
        double density(std::vector<double>);
    };

    Distribution uniform_distribution(std::vector<double> bounds);
}

#endif
