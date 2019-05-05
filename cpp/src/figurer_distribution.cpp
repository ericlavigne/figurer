#include "figurer_distribution.hpp"

namespace figurer {

    Distribution::Distribution() {}

    void Distribution::set_dimension(int dimension) {
        dimension_ = dimension;
    }

    void Distribution::set_seed_dimension(int seed_dimension) {
        seed_dimension_ = seed_dimension;
    }

    void Distribution::set_sample_fn(std::function<std::vector<double>(std::vector<double>)> sample_fn) {
        sample_fn_ = sample_fn;
    }

    void Distribution::set_density_fn(std::function<double(std::vector<double>)> density_fn) {
        density_fn_ = density_fn;
    }

    std::vector<double> Distribution::sample() {
        int size = seed_dimension_ >= 0 ? seed_dimension_ : dimension_;
        if (size < 0) {
            throw std::invalid_argument("Need to set seed_dimension before sampling");
        }
        std::vector<double> seed(size);
        for (int i = 0; i < size; i++) {
            seed.push_back(((double) rand()) / RAND_MAX);
        }
        return sample_fn_(seed);
    }

    double Distribution::density(std::vector<double> coordinates) {
        return density_fn_(coordinates);
    }

}
