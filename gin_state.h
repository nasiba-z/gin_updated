#ifndef GIN_STATE_H
#define GIN_STATE_H

#include <cstddef>


constexpr size_t BLCKSZ = 8192; // Used here as the maximum inline posting list size (in bytes)

struct GinState {
    bool isBuild;           // Whether we are in build mode
    size_t maxItemSize;     // Maximum allowed size (in bytes) for an inline posting list

    GinState(bool build = true, size_t maxSize = BLCKSZ)
        : isBuild(build), maxItemSize(maxSize) {}
};

#endif // GIN_STATE_H