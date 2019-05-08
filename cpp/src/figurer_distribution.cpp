#include "figurer_distribution.hpp"

namespace figurer {

    Distribution::Distribution() {
        dimension_ = -1;
        seed_dimension_ = -1;
    }

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
            seed[i] = ((double) rand()) / RAND_MAX;
        }
        return sample_fn_(seed);
    }

    double Distribution::density(std::vector<double> coordinates) {
        return density_fn_(coordinates);
    }

    Distribution uniform_distribution(std::vector<double> bounds) {
        size_t size = bounds.size();
        size_t dimension = size / 2;
        if(size != dimension * 2) {
            throw std::invalid_argument("Bounds must have even size");
        }
        Distribution distribution;
        distribution.set_dimension(dimension);
        distribution.set_sample_fn([dimension, bounds](std::vector<double> seed) {
            std::vector<double> result(dimension,0.0);
            for(int i = 0; i < dimension; i++) {
                result[i] = bounds[i*2] + seed[i] * (bounds[i*2+1] - bounds[i*2]);
            }
            return result;
        });
        distribution.set_density_fn([dimension, bounds](std::vector<double> val) {
            for(int i = 0; i < dimension; i++) {
                if((val[i] < bounds[i*2]) || (val[i] > bounds[i*2+1])) {
                    return 0.0;
                }
            }
            return 1.0;
        });
        return distribution;
    }
}
