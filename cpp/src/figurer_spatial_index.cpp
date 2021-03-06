#include "figurer_spatial_index.hpp"
#include <cmath>
#include <string>

namespace figurer {

    spatial_index::spatial_index() : dimension_{-1}, data_{} {}

    spatial_index::spatial_index(int dimension) : dimension_{dimension}, data_{} {}

    void spatial_index::add(int id, std::vector<double> position) {
        if(dimension_ < 0) {
            dimension_ = position.size();
        }
        if(position.size() == dimension_) {
            data_.emplace_back(id,position);
        } else {
            throw std::invalid_argument("Adding vector of dimension " + std::to_string(position.size()) +
                                        " to spatial_index of dimension " + std::to_string(dimension_));
        }
    }

    std::pair<int,std::vector<double>> spatial_index::closest(const std::vector<double>& position) {
        if(data_.empty()) {
            throw std::invalid_argument("Can't find closest point in empty data set");
        }
        if(position.size() != dimension_) {
            throw std::invalid_argument("Searching for vector of dimension " + std::to_string(position.size()) +
                                        " tin spatial_index of dimension " + std::to_string(dimension_));
        }
        int closest_index = 0;
        double closest_distance = distance2(position, data_[0].second);
        for(int i = 1; i < data_.size(); i++) {
            double dist = distance2(position, data_[i].second);
            if(dist < closest_distance) {
                closest_distance = dist;
                closest_index = i;
            }
        }
        return data_[closest_index];
    }

    double spatial_index::closest_distance(const std::vector<double>& position) {
        return sqrt(closest_distance2(position));
    }

    double spatial_index::closest_distance2(const std::vector<double>& position) {
        auto found = closest(position);
        return distance2(position, found.second);
    }

    int spatial_index::size() {
        return data_.size();
    }

    double distance(const std::vector<double>& position1, const std::vector<double>& position2) {
        return sqrt(distance2(position1,position2));
    }

    double distance2(const std::vector<double>& position1, const std::vector<double>& position2) {
        if(position1.size() != position2.size()) {
            throw std::invalid_argument("Can't compute L2 distance between vector of dimension " + std::to_string(position1.size()) +
                                        " and vector of dimension " + std::to_string(position2.size()));
        }
        double result = 0;
        for(int i = 0; i < position1.size(); i++) {
            result += pow(position1[i] - position2[i], 2.0);
        }
        return result;
    }
}
