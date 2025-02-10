#include "gin_posting_list.h"
#include <cstring>

GinPostingList* createGinPostingList(const std::vector<TID>& tids){
    if (tids.empty())
        return nullptr;
    TID first = tids[0];
    size_t numRemaining = tids.size() - 1;
    size_t nbytes = numRemaining * sizeof(TID);
    size_t totalSize = sizeof(GinPostingList) - 1 + nbytes;
    auto* list = reinterpret_cast<GinPostingList*>(new unsigned char[totalSize]);
    list->first = first;
    list->nbytes = static_cast<std::uint16_t>(nbytes);
    if (numRemaining > 0)
        std::memcpy(list->bytes, &tids[1], nbytes);
    return list;
};

std::vector<TID> decodeGinPostingList(const GinPostingList* list){
    std::vector<TID> result;
    if (!list)
        return result;
    result.push_back(list->first);
    size_t numRemaining = list->nbytes / sizeof(TID);
    for (size_t i = 0; i < numRemaining; ++i) {
        TID tid;
        std::memcpy(&tid, list->bytes + i * sizeof(TID), sizeof(TID));
        result.push_back(tid);
    }
    return result;
}

