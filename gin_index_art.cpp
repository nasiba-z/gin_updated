#include "gin_index_art.h"
#include "posting_tree.h"
#include <stdexcept>
#include <algorithm>
#include <cstring>

// ----------------------------------------------------------------
// Implementation of createGinPostingList for in-memory use.
// It creates a new GinPostingList using the postingData vector.
GinPostingList* createGinPostingList(const std::vector<TID>& postingData) {
    // Simply allocate a new GinPostingList from the vector.
    return new GinPostingList(postingData);
}

// ----------------------------------------------------------------
// getPostingList: Retrieve the posting list from an IndexTuple.
std::vector<TID> getPostingList(IndexTuple* tup) {
    std::vector<TID> result;
    if (tup->postingList) {
        result = tup->postingList->tids;
    } else if (tup->postingTree) {
        // Assume postingTree has a method getTIDs() that returns a vector<TID>.
        result = tup->postingTree->getTIDs();
    }
    return result;
}
// ----------------------------------------------------------------
// Implementation of addPostingList member function.
// Instead of copying, we transfer ownership directly.
void IndexTuple::addPostingList(const GinPostingList& plist) {
    // Create a new GinPostingList from the provided one.
    // Since we are in-memory, we can simply use the copy constructor.
    postingList.reset(new GinPostingList(plist));
}

size_t IndexTuple::getTupleSize() const {
    // Compute the base size: size of key,  and postingSize.
    size_t baseSize = sizeof(key)  + sizeof(postingSize);
    
    // If we have an inline posting list, add its "logical" size.
    if (postingList) {
        // We simulate the size as the fixed overhead plus the dynamic data.
        size_t plSize = sizeof(GinPostingList) + postingList->tids.size() * sizeof(TID);
        return baseSize + plSize;
    } else if (postingTree) {
        // If a posting tree is used, we assume PostingTree has a method getTotalSize() that
        // returns the total size in bytes of the posting tree.
        return baseSize + postingTree->getTotalSize();
    } else {
        return baseSize;
    }
}

// ----------------------------------------------------------------
// GinFormTuple_ART: Create an IndexTuple from the given key and posting data,
// optimized for the ART-backed posting tree.
// ----------------------------------------------------------------
IndexTuple* GinFormTuple_ART(GinState* ginstate,
    datum key,
    const std::vector<TID>& postingData,
    bool errorTooBig)
    {   // Ensure the key is not null
        // if (key.empty()) {
        //     throw std::logic_error("Key cannot be null");
        // }
        // Create a new index tuple.
        IndexTuple* itup = new IndexTuple();
        // Set the key directly (no conversion needed).
        itup->key = key;

        size_t numTIDs = postingData.size();
        if (numTIDs == 0) {
            // No posting data; return tuple as is.
            return itup;
        }
        // If the number of TIDs exceeds the inline threshold (GinMaxItemSize),
        // convert the inline posting list into a posting tree.
        if (postingData.size() > GinMaxItemSize) {
            // Create a new PostingTree.
            PostingTree* tree = new PostingTree();
            // Instead of bulk-loading, we mimic GIN's behavior by first filling
            // an inline portion and then inserting remaining TIDs incrementally.
            tree->bulkLoad(postingData);
            // Store the posting tree pointer in the tuple.
            itup->postingTree = tree;
            itup->postingSize = postingData.size();
        } 
    
        else {
            // Build an inline posting list.
            GinPostingList* inlineList = createGinPostingList(postingData);
            if (!inlineList) {
            delete itup;
            return nullptr;
        }
        // Transfer ownership of inlineList to the tuple's smart pointer.
        itup->postingList.reset(inlineList);
        itup->postingSize = numTIDs;
        
         } 
        return itup;
    }
