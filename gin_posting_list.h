#ifndef GIN_POSTING_LIST_H
#define GIN_POSTING_LIST_H

#include "tid.h"
#include <vector>
#include <cstdint>


struct GinPostingList {
    TID first;               // First TID stored uncompressed
    std::uint16_t nbytes;    // Number of bytes that follow
    TID bytes[1];             // Flexible array member, placeholder of 1 TID for uncompressed items

    size_t getSizeInBytes() const {
        return sizeof(GinPostingList) - 1 + nbytes;
    }
};


GinPostingList* createGinPostingList(const std::vector<TID>& tids);

std::vector<TID> decodeGinPostingList(const GinPostingList* list);
struct GinPostingListDeleter {
    void operator()(GinPostingList* ptr) const {
        delete[] reinterpret_cast<unsigned char*>(ptr);
    }
};
#endif // GIN_POSTING_LIST_H