#ifndef GIN_INDEX_H
#define GIN_INDEX_H

#include <vector>
#include <unordered_map>
#include <memory>   
#include <cstdint>
#include "posting_tree.h"
#include "gin_posting_list.h"  
#include "posting_tree.h"
#include "gin_state.h"


struct GinIndex {
    std::unordered_map<std::int32_t, std::vector<GinPostingList*>> index;

    void insert(const std::int32_t& key, const std::vector<TID>& tids) {
        GinPostingList* plist = createGinPostingList(tids);
        index[key].push_back(plist);
    }

    ~GinIndex() {
        for (auto& kv : index) {
            for (auto* plist : kv.second) {
                delete[] reinterpret_cast<unsigned char*>(plist);
            }
        }
    }
};


struct IndexTuple {
    public:
        std::vector<std::string> datums; // Data entries (e.g., the key as a string)
        std::unique_ptr<GinPostingList, GinPostingListDeleter> postingList; // Inline posting list
        // Instead of a raw GinPage, we now store a pointer to a PostingTree if needed.
        PostingTree* postingTree = nullptr;
        size_t postingOffset = 0;
        size_t postingSize = 0;
    
        // Default constructor.
        IndexTuple() = default;
    
        // Copy constructor.
        IndexTuple(const IndexTuple& other)
            : datums(other.datums),
              postingList(other.postingList ? new GinPostingList(*other.postingList) : nullptr),
              postingTree(other.postingTree), // Shallow copy; for deep copy, you'd need to copy the tree.
              postingOffset(other.postingOffset),
              postingSize(other.postingSize) {}
    
        // Copy assignment operator.
        IndexTuple& operator=(const IndexTuple& other) {
            if (this == &other)
                return *this;
            datums = other.datums;
            postingList.reset(other.postingList ? new GinPostingList(*other.postingList) : nullptr);
            postingTree = other.postingTree;
            postingOffset = other.postingOffset;
            postingSize = other.postingSize;
            return *this;
        }
    
        // Move constructor and move assignment operator.
        IndexTuple(IndexTuple&& other) noexcept = default;
        IndexTuple& operator=(IndexTuple&& other) noexcept = default;
    
        // Adds an inline posting list to the tuple.
        void addPostingList(const GinPostingList& plist) {
            postingList.reset(new GinPostingList(plist));
        }
    
        // Returns the total size of the tuple (for demonstration).
        size_t getTupleSize() const {
            size_t size = sizeof(IndexTuple);
            if (postingList)
                size += postingList->getSizeInBytes();
            return size;
        }
    };

std::vector<IndexTuple> ginFormTuple(const GinIndex& gin, const GinState& state);


#endif // GIN_INDEX_H