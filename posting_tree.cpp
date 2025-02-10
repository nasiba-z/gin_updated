#include "ginbtree_node.h"
#include "tid.h"
#include "gin_posting_list.h"
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include "posting_tree.h" 
#include <cstdint>


struct GinPostingList;


PostingTree* createPostingTree(const std::vector<TID>& tids) {
    PostingTree* tree = new PostingTree(2); // Provide the minimum degree as an argument
    // For example, assume each segment can hold at most 128 TIDs.
    // (In PostgreSQL, the maximum segment size is determined by GinPostingListSegmentMaxSize.)
    size_t maxTIDsPerSegment = 128;
    size_t total = tids.size();
    for (size_t i = 0; i < total; i += maxTIDsPerSegment) {
        size_t count = std::min(maxTIDsPerSegment, total - i);
        std::vector<TID> segment(tids.begin() + i, tids.begin() + i + count);
        GinPostingList* plist = createGinPostingList(segment);
        if (plist)
            tree->segments.push_back(plist);
    }
    return tree;
}