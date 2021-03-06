#ifndef FIGURER_FIGURER_SPATIAL_INDEX_HPP
#define FIGURER_FIGURER_SPATIAL_INDEX_HPP

#include <vector>

namespace figurer {
    class spatial_index {
        int dimension_;
        std::vector<std::pair<int,std::vector<double>>> data_;
    public:
        spatial_index();
        spatial_index(int dimension);
        void add(int id, std::vector<double> position);
        std::pair<int,std::vector<double>> closest(const std::vector<double>& position);
        double closest_distance(const std::vector<double>& position);
        double closest_distance2(const std::vector<double>& position);
        int size();
    };
    double distance(const std::vector<double>& position1,  const std::vector<double>& position2);
    double distance2(const std::vector<double>& position1,  const std::vector<double>& position2);
}

#endif
