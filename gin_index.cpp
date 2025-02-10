#include "ginbtree_node.h"
#include "tid.h"
#include "gin_posting_list.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include "posting_tree.h"
#include "gin_index.h"
#include "gin_state.h"


std::vector<IndexTuple> ginFormTuple(const GinIndex& gin, const GinState& state) {
    std::vector<IndexTuple> tuples;
    for (const auto& kv : gin.index) {
        const std::string key = std::to_string(kv.first);
        const auto& lists = kv.second;

        // Merge posting lists for this key.
        std::vector<TID> mergedTIDs;
        for (const auto* plist : lists) {
            auto tids = decodeGinPostingList(plist);
            mergedTIDs.insert(mergedTIDs.end(), tids.begin(), tids.end());
        }
        // Remove duplicates.
        std::sort(mergedTIDs.begin(), mergedTIDs.end(), [](const TID& a, const TID& b) {
            return (a.rowId < b.rowId) ;
        });
        mergedTIDs.erase(std::unique(mergedTIDs.begin(), mergedTIDs.end(), [](const TID& a, const TID& b) {
            return a.rowId == b.rowId;
        }), mergedTIDs.end());

        if (mergedTIDs.empty())
            continue;

        // Compute what the inline posting list size would be.
        size_t plSize = sizeof(GinPostingList) - 1 + ((mergedTIDs.size() - 1) * sizeof(TID));

        IndexTuple tuple;
        tuple.datums.push_back(key);

        // If too many TIDs (or size exceeds maximum), use a posting tree.
        if (mergedTIDs.size() > 20 || plSize > state.maxItemSize) {
            PostingTree* tree = createPostingTree(mergedTIDs);
            tuple.postingTree = tree;
            tuple.postingOffset = 0;
            tuple.postingSize = mergedTIDs.size();
        } else {
            // Otherwise, build an inline posting list.
            GinPostingList* combined = createGinPostingList(mergedTIDs);
            if (!combined)
                continue;
            tuple.addPostingList(*combined);
            delete[] reinterpret_cast<unsigned char*>(combined);
        }
        tuples.push_back(std::move(tuple));
    }
    return tuples;
}