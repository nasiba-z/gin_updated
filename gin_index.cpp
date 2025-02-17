#include "gin_index.h"
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
// GinFormTuple: Create an IndexTuple from the given key and posting data.
IndexTuple* GinFormTuple(GinState* ginstate,
                         datum key,
                         const std::vector<TID>& postingData,
                         bool errorTooBig)
{
    // Create a new index tuple.
    IndexTuple* itup = new IndexTuple();
    // Set the key directly (no conversion needed).
    itup->key = key;
    
    // Compute the "inline posting list size" (simulate the disk layout size calculation).
    // Here we assume an inline posting list would have a header size of:
    // sizeof(GinPostingList) minus the size of one TID, plus sizeof(TID) for each extra TID.
    size_t numTIDs = postingData.size();
    if (numTIDs == 0) {
        // No posting data; return tuple as is.
        return itup;
    }
    size_t plSize = sizeof(GinPostingList) + numTIDs * sizeof(TID);
    
    // Instead of throwing an error, if the inline posting list is too big, we build a PostingTree.
    if (postingData.size() > LeafMaxCount) {
        // Build a PostingTree using bulk-loading.
        PostingTree* tree = new PostingTree();
        // We assume postingData is already sorted.
        tree->bulkLoad(postingData);
        // Store the posting tree pointer in the tuple.
        itup->postingTree = tree;
        itup->postingSize = postingData.size();
    } else {
        // Build an inline posting list.
        GinPostingList* inlineList = createGinPostingList(postingData);
        if (!inlineList) {
            delete itup;
            return nullptr;
        }
        // Transfer ownership of inlineList to the tuple's smart pointer.
        itup->postingList.reset(inlineList);
        itup->postingSize = postingData.size();
    }
    
    return itup;
}