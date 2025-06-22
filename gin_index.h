#ifndef GIN_INDEX_H
#define GIN_INDEX_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "posting_tree.h"
#include "gin_state.h"
#include "tid.h"
using datum = int32_t; 
class GinIndex {
public:
    std::unordered_map<datum, std::vector<TID>> index;

    GinIndex(const GinState& state) : state(state) {}

    // Insert posting data for a key.
    void insert(datum key, const std::vector<TID>& tids) {
        auto it = index.find(key);
        if (it != index.end()) {
            // Append new TIDs to the existing vector.
            it->second.insert(it->second.end(), tids.begin(), tids.end());
        } else {
            // Create a new posting list.
            index[key] = tids;
        }
    }
    GinState state;
};


// In-memory GinPostingList stores TIDs using a std::vector.
struct GinPostingList {
    std::vector<TID> tids;
    GinPostingList() = default;
    explicit GinPostingList(const std::vector<TID>& inputTIDs)
        : tids(inputTIDs) { }
    
    // Add a TID to the posting list.
    void addTID(const TID& tid) {
        tids.push_back(tid);
    }
    
    // Return the number of TIDs.
    size_t size() const {
        return tids.size();
    }
};

// Forward declaration of PostingTree.
class PostingTree;

// Custom deleter for GinPostingList.
struct GinPostingListDeleter {
    void operator()(GinPostingList* p) const {
        delete p;
    }
};

// IndexTuple represents a tuple in the GIN index.
// For our in-memory version, the key is stored as an integer (Datum),
// and the posting list is stored either inline or as a pointer to a PostingTree.
struct IndexTuple {
    datum key; // The key (an integer representing a trigram).
    
    // Inline posting list: if the posting list is small enough,
    // it is stored in this smart pointer.
    std::unique_ptr<GinPostingList, GinPostingListDeleter> postingList;
    
    // If the inline posting list is too big, a posting tree is used.
    // This pointer is valid only if postingTree is not nullptr.
    PostingTree* postingTree;
    
    // Additional metadata.
    size_t postingSize;   // Number of TIDs in the posting list.
    
    // Default constructor.
    IndexTuple() : key(0), postingTree(nullptr),  postingSize(0) { }
    
    // Member function to add an inline posting list.
    void addPostingList(const GinPostingList& plist);
    
    // Member function to compute the size of the tuple in bytes.
    // (For in-memory use, you can simply simulate this if needed.)
    size_t getTupleSize() const;
};

// Form a tuple from a key and its posting list data.
// If the posting list is too large, a PostingTree is built instead.
IndexTuple* GinFormTuple(GinState* ginstate,
                         datum key,
                         const std::vector<TID>& postingData,
                         bool errorTooBig);

// Declaration of createGinPostingList: builds an in-memory posting list from postingData.
GinPostingList* createGinPostingList(const std::vector<TID>& postingData);
std::vector<TID> getPostingList(IndexTuple* tup);

#endif // GIN_INDEX_H