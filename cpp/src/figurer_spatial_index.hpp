#ifndef FIGURER_FIGURER_SPATIAL_INDEX_HPP
#define FIGURER_FIGURER_SPATIAL_INDEX_HPP

namespace figurer {
    class SpatialIndex {
        //flann::Index<flann::L2<double>> index_;
    public:
        SpatialIndex(int dimension);
    };
    SpatialIndex spatial_index(int dimension);
}

#endif
