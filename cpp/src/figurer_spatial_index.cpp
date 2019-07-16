#include "figurer_spatial_index.hpp"

namespace figurer {

    SpatialIndex::SpatialIndex(int dimension)
    : index_{flann::KDTreeIndexParams(4)}
    {

    }
    SpatialIndex spatial_index(int dimension) {
        SpatialIndex result(dimension);
        return result;
    }
}
