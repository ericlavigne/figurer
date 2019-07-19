#include <IndexFlat.h>
#include "figurer_spatial_index.hpp"

namespace figurer {

    SpatialIndex::SpatialIndex(int dimension)
    {
        faiss::IndexFlatL2 index(dimension);
    }
    SpatialIndex spatial_index(int dimension) {
        SpatialIndex result(dimension);
        return result;
    }
}
